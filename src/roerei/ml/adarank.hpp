#pragma once

#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>
#include <roerei/performance.hpp>

#include <roerei/generic/compact_sparse_matrix.hpp>
#include <roerei/generic/full_matrix.hpp>
#include <roerei/generic/encapsulated_array.hpp>
#include <roerei/generic/encapsulated_vector.hpp>
#include <roerei/generic/id_t.hpp>
#include <roerei/generic/set_operations.hpp>

#include <roerei/generic/common.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>

namespace roerei
{

class adarank
{
private:
	struct t_t : public id_t<t_t>
	{
		t_t(size_t _id) : id_t<t_t>(_id) {}
	};

	struct ir_feature_id_t : public id_t<ir_feature_id_t>
	{
		ir_feature_id_t(size_t _id) : id_t<ir_feature_id_t>(_id) {}
	};

public:
	static constexpr size_t ir_feature_size = 7;

	typedef object_id_t query_id_t;
	typedef dependency_id_t document_id_t;

	typedef std::vector<std::pair<document_id_t, float>> ranking_t;
	typedef encapsulated_array<ir_feature_id_t, float, ir_feature_size> feature_vector_t;

	struct feature_requirements_t {
		compact_sparse_matrix_t<document_id_t, feature_id_t, float> document_query_summary;
		encapsulated_vector<feature_id_t, float> idf;
		encapsulated_vector<feature_id_t, float> cwic;

		encapsulated_vector<document_id_t, float> d_sums;
		float C_sum;
	};

private:
	struct intermediaries_t {
		std::vector<query_id_t> queries;
		full_matrix_t<query_id_t, document_id_t, feature_vector_t> features;
		full_matrix_t<ir_feature_id_t, query_id_t, float> E_weak_cached;
		full_matrix_t<t_t, query_id_t, float> p;
	};

private:
	size_t const T;
	dataset_t const& d;

	encapsulated_vector<t_t, ir_feature_id_t> h;
	encapsulated_vector<t_t, float> alpha;

private:
	template<typename MATRIX>
	static sparse_matrix_t<document_id_t, feature_id_t, float> create_dqs(dataset_t const& d, MATRIX const& trainingset)
	{
		sparse_matrix_t<document_id_t, feature_id_t, float> document_query_summary(d.dependencies.size(), d.features.size());

		auto dependants = dependencies::create_dependants(d);
		d.dependencies.keys([&](dependency_id_t dep_id) {
			for (object_id_t obj_id : dependants[dep_id]) {
				try {
					for (std::pair<feature_id_t, float> kvp : trainingset[obj_id]) {
						if (kvp.second > 0.0f) {
							document_query_summary[dep_id][kvp.first] += kvp.second;
						}
					}
				} catch (std::out_of_range) {
					// Do nothing
				}
			}
		});

		return document_query_summary;
	}

	static encapsulated_vector<feature_id_t, float> create_idf(dataset_t const& d, decltype(feature_requirements_t::document_query_summary) const& dqs)
	{
		encapsulated_vector<feature_id_t, size_t> frequencies(d.features.size());
		dqs.citerate([&](auto const& row) {
			for (std::pair<feature_id_t, float> kvp : row) {
				if (kvp.second > 0.0f) {
					frequencies[kvp.first]++;
				}
			}
		});

		encapsulated_vector<feature_id_t, float> idf(d.features.size());
		float N = d.dependencies.size();
		feature_id_t::iterate([&](feature_id_t fid) {
			idf[fid] = std::log(N / static_cast<float>(frequencies[fid] + 1));
		}, d.features.size());

		return idf;
	}

	static encapsulated_vector<feature_id_t, float> create_cwic(dataset_t const& d, decltype(feature_requirements_t::document_query_summary) const& dqs)
	{
		encapsulated_vector<feature_id_t, float> cwic(d.features.size());
		dqs.citerate([&](auto const& row) {
			for (std::pair<feature_id_t, float> const& kvp : row) {
				cwic[kvp.first] += kvp.second;
			}
		});
		return cwic;
	}

public:
	template<typename MATRIX>
	static feature_requirements_t create_feature_requirements(dataset_t const& d, MATRIX const& trainingset) {
		auto dqs = create_dqs(d, trainingset);
		auto idf = create_idf(d, dqs);
		auto cwic = create_cwic(d, dqs);

		float C_sum = 0.0f;
		cwic.iterate([&](feature_id_t, float x) {
			C_sum += x;
		});

		encapsulated_vector<document_id_t, float> d_sums;
		d_sums.reserve(d.dependencies.size());
		d.dependencies.keys([&](document_id_t d_id) {
			auto const& document(dqs[d_id]);
			float d_sum = 0.0f;
			for (auto const& kvp : document) {
				d_sum += kvp.second;
			}
			d_sums.emplace_back(d_sum);
		});

		return {
			std::move(dqs),
			std::move(idf),
			std::move(cwic),
			std::move(d_sums),
			C_sum,
		};
	}

private:
	template<typename FEATURES>
	feature_vector_t compute_features(feature_requirements_t const& fr, FEATURES const& query, document_id_t d_id) const {
		auto document = fr.document_query_summary[d_id];

		float d_sum = fr.d_sums[d_id];

		float frequency_sum = 0.0f;
		float cwic_div_sum = 0.0f;
		float idf_sum = 0.0f;
		float cwid_frac_sum = 0.0f;
		float cwid_frac_idf_sum = 0.0f;
		float cwid_cwic_sum = 0.0f;
		float bmtwentyfive = 0.0f;

		float constexpr kone = 1.5f;
		float constexpr b = 0.75f;

		float const fraction = kone * (1.0f - b + b * (d_sum / (fr.C_sum / static_cast<float>(fr.document_query_summary.size_m()))));
		set_compute_smart_intersect(
			document.begin(), document.end(), get_nonempty_size(document),
			query.begin(), query.end(), get_nonempty_size(query),
			[](auto const& df) { return df.first; },
			[](auto const& qf) { return qf.first; },
			[&](auto const& df, auto const& /*qf*/) {
				frequency_sum += std::log1p(df.second);
				cwic_div_sum += std::log1p(fr.C_sum / fr.cwic[df.first]);
				idf_sum += std::log(fr.idf[df.first]);
				cwid_frac_sum += std::log1p(df.second / d_sum);
				cwid_frac_idf_sum += std::log1p(df.second / d_sum + fr.idf[df.first]);
				cwid_cwic_sum += std::log1p( (df.second * fr.C_sum) / (d_sum * fr.cwic[df.first]));

				bmtwentyfive +=
						fr.idf[df.first] *
						(
							(df.second * (kone + 1.0f)) /
							(df.second + fraction)
						);
			}
		);

		return feature_vector_t(
			frequency_sum,
			cwic_div_sum,
			idf_sum,
			cwid_frac_sum,
			cwid_frac_idf_sum,
			cwid_cwic_sum,
			std::log(bmtwentyfive)
		);
	}

	float compute_E(ranking_t const& ranking, object_id_t test_row_i) const
	{
		return performance::measure_oocover(d, test_row_i, ranking); // Metric we want to maximize
	}

	template<typename FEATURES>
	float compute_f(t_t t, FEATURES const& feature_vector) const
	{
		float result = 0.0f;
		t_t::iterate([&](t_t k) {
			result += alpha[k] * feature_vector[h[k]];
		}, t.unseal()+1);
		return result;
	}

	ranking_t create_ranking_weak(intermediaries_t const& inter, query_id_t q_id, ir_feature_id_t k) const
	{
		ranking_t ranking;
		d.dependencies.keys([&](dependency_id_t d_id) {
			ranking.emplace_back(std::make_pair(d_id, inter.features[q_id][d_id][k]));
		});
		return ranking;
	}

	ranking_t create_ranking_strong(intermediaries_t const& inter, query_id_t q_id, t_t t) const
	{
		ranking_t ranking;
		d.dependencies.keys([&](dependency_id_t d_id) {
			ranking.emplace_back(std::make_pair(d_id, compute_f(t, inter.features[q_id][d_id])));
		});
		return ranking;
	}

	ir_feature_id_t compute_h(intermediaries_t const& inter, t_t t) const
	{
		ir_feature_id_t result_k = ir_feature_size; // Non-existing feature
		float result_score = std::numeric_limits<float>::lowest();

		encapsulated_array<ir_feature_id_t, float, ir_feature_size> scores;
		ir_feature_id_t::iterate([&](ir_feature_id_t k) {
			bool exists = false;
			t_t::iterate([&](t_t u) {
				if (h[u] == k) {
					exists = true;
				}
			}, t.unseal());

			if (exists) {
				return; // Skip!
			}

			float sum  = 0;
			for (query_id_t i : inter.queries) {
				sum += inter.p[t][i] * inter.E_weak_cached[k][i];
			}
			scores[k] = sum;

			if (result_score < sum) {
				result_score = sum;
				result_k = k;
			}
		}, ir_feature_size);

		std::stringstream sstr;
		sstr << '{';
		ir_feature_id_t::iterate([&](ir_feature_id_t k) {
			sstr << scores[k] << ',';
		}, ir_feature_size);
		sstr << '}';
		std::cerr << sstr.str() << std::endl;

		return result_k;
	}

	float compute_alpha(intermediaries_t const& inter, t_t t) const
	{
		float a = 0.0f;
		float b = 0.0f;

		for (query_id_t i : inter.queries) {
			float e = inter.E_weak_cached[h[t]][i];
			float pti = inter.p[t][i];

			a += pti * (1 + e);
			b += pti * (1 - e);
		}

		return 0.5f * std::log(a / b);
	}

public:
	adarank(adarank const&) = delete;
	adarank(adarank&&) = default;

	template<typename ORIG_MATRIX>
	adarank(
			size_t _T,
			dataset_t const& _d,
			ORIG_MATRIX const& trainingset
			)
		: T(_T)
		, d(_d)
		, h(T, ir_feature_size) // Initialize with non existing feature
		, alpha(T)
	{
		auto fr = create_feature_requirements(d, trainingset);

		std::cout << "Requirements computed" << std::endl;

		intermediaries_t inter{
			std::vector<query_id_t>(),
			full_matrix_t<query_id_t, document_id_t, feature_vector_t>(d.objects.size(), d.dependencies.size()),
			full_matrix_t<ir_feature_id_t, query_id_t, float>(ir_feature_size, d.objects.size()),
			full_matrix_t<t_t, query_id_t, float>(T, d.objects.size())
		};

		std::vector<std::pair<feature_id_t, float>> query_row;
		trainingset.citerate([&](auto const& original_row) {
			query_row.clear();

			for (auto const& kvp : original_row) {
				query_row.emplace_back(kvp);
			}

			auto&& row = inter.features[original_row.row_i];
			d.dependencies.keys([&](document_id_t d_id) {
				row[d_id] = compute_features(fr, query_row, d_id);
			});

			inter.queries.emplace_back(original_row.row_i);
		});

		ir_feature_id_t::iterate([&](ir_feature_id_t k) {
			for (query_id_t i : inter.queries) {
				inter.E_weak_cached[k][i] = compute_E(create_ranking_weak(inter, i, k), i);
			}
		}, ir_feature_size);

		const float m = inter.queries.size();
		for (query_id_t i : inter.queries) {
			inter.p[0][i] = 1.0f / m;
		}

		t_t::iterate([&](t_t t) {
			h[t] = compute_h(inter, t);
			alpha[t] = compute_alpha(inter, t);

			std::cout
				<< "h[" << t.unseal() << "]: " << h[t].unseal() << '\n'
				<< "alpha[" << t.unseal() << "]: " << alpha[t] << std::endl;

			if (t.unseal()+1 >= T) {
				return;
			}

			encapsulated_vector<object_id_t, float> strong_E(d.objects.size());
			float sum_strong_E = 0.0f;
			for (query_id_t i : inter.queries) {
				float sei = std::exp(-1.0f * compute_E(create_ranking_strong(inter, i, t), i));
				strong_E[i] = sei;
				sum_strong_E += sei;
			}
			std::cout << "sum strong_E: " << sum_strong_E << std::endl;

			for (query_id_t i : inter.queries) {
				inter.p[t_t(t.unseal()+1)][i] = strong_E[i] / sum_strong_E;
			}
		}, T);
	}

	template<typename ROW, typename TRAININGSET>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& test_row, TRAININGSET const& trainingset) const
	{
		feature_requirements_t fr(create_feature_requirements(d, trainingset));

		ranking_t ranking;

		d.dependencies.keys([&](document_id_t d_id) {
			float f = compute_f(T-1, compute_features(fr, test_row, d_id));
			if (f >= 0.0f) {
				ranking.emplace_back(std::make_pair(d_id, f));
			}
		});

		return ranking;
	}
};

}

