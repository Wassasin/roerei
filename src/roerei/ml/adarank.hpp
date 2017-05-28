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

#include <roerei/util/fast_log.hpp>

#include <roerei/generic/common.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>

namespace roerei
{

template<typename MATRIX>
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

	typedef object_id_t query_id_t;
	typedef dependency_id_t document_id_t;

	static constexpr size_t ir_feature_size = 2;

	typedef std::vector<std::pair<document_id_t, float>> ranking_t;
	typedef encapsulated_array<ir_feature_id_t, float, ir_feature_size> feature_vector_t;

private:
	size_t const T;
	dataset_t const& d;
	MATRIX const& trainingset;

	//sparse_matrix_t<dependency_id_t, object_id_t, feature_vector_t> x;

	compact_sparse_matrix_t<document_id_t, feature_id_t, float> document_query_summary;
	encapsulated_vector<feature_id_t, float> idf;

	full_matrix_t<query_id_t, document_id_t, feature_vector_t> features;

	full_matrix_t<ir_feature_id_t, query_id_t, float> E_cached;
	full_matrix_t<t_t, query_id_t, float> p;

	encapsulated_vector<t_t, ir_feature_id_t> h;
	encapsulated_vector<t_t, float> alpha;

private:
	static sparse_matrix_t<document_id_t, feature_id_t, float> create_dqs(dataset_t const& d, MATRIX const& trainingset) {
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

	void init_idf()
	{
		encapsulated_vector<feature_id_t, size_t> frequencies(d.features.size());
		document_query_summary.citerate([&](auto const& row) {
			for (std::pair<feature_id_t, float> kvp : row) {
				if (kvp.second > 0.0f) {
					frequencies[kvp.first]++;
				}
			}
		});

		float N = d.dependencies.size();
		feature_id_t::iterate([&](feature_id_t fid) {
			idf[fid] = std::log(N / static_cast<float>(frequencies[fid] + 1));
		}, d.features.size());
	}

	template<typename FEATURES>
	feature_vector_t compute_features(FEATURES const& query, document_id_t d_id) const {
		auto document = document_query_summary[d_id];

		float frequency_sum = 0.0f;
		float idf_sum = 0.0f;

		set_compute_intersect(
			document.begin(), document.end(),
			query.begin(), query.end(),
			[](auto const& df) { return df.first; },
			[](auto const& qf) { return qf.first; },
			[&](auto const& df, auto const& /*qf*/) {
				frequency_sum += fast_log(1.0f + df.second);
				idf_sum += fast_log(idf[df.first]);
			}
		);

		return feature_vector_t(
			frequency_sum,
			idf_sum
		);
	}

	float compute_E(ranking_t const& ranking, object_id_t test_row_i) const
	{
		return performance::measure_oocover(d, test_row_i, ranking); // Metric we want to maximize
	}

	ranking_t create_ranking_weak(object_id_t q_id, ir_feature_id_t k) const
	{
		ranking_t ranking;
		d.dependencies.keys([&](dependency_id_t d_id) {
			ranking.emplace_back(std::make_pair(d_id, features[q_id][d_id][k]));
		});
		return ranking;
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

	ranking_t create_ranking_strong(object_id_t q_id, t_t t) const
	{
		ranking_t ranking;
		d.dependencies.keys([&](dependency_id_t d_id) {
			ranking.emplace_back(std::make_pair(d_id, compute_f(t, features[q_id][d_id])));
		});
		return ranking;
	}

	ir_feature_id_t compute_h(t_t t) const
	{
		ir_feature_id_t result_k = ir_feature_size; // Non-existing feature
		float result_score = std::numeric_limits<float>::lowest();

		ir_feature_id_t::iterate([&](ir_feature_id_t k) {
			float sum  = 0;
			trainingset.citerate([&](typename MATRIX::const_row_proxy_t const& row) {
				const object_id_t i = row.row_i;
				sum += p[t][i] * E_cached[k][i];
			});

			std::cout << "score " << k.unseal() << " " << sum << std::endl;
			if (result_score < sum) {
				result_score = sum;
				result_k = k;
			}
		}, ir_feature_size);

		return result_k;
	}

	float compute_alpha(t_t t) const
	{
		float a = 0.0f;
		float b = 0.0f;

		trainingset.citerate([&](auto const& row) {
			object_id_t i = row.row_i;
			float e = E_cached[h[t]][i];

			a += p[t][i] * (1 + e);
			b += p[t][i] * (1 - e);
		});

		return 0.5f * std::log(a / b);
	}

public:
	adarank(
			size_t _T,
			dataset_t const& _d,
			MATRIX const& _trainingset
			)
		: T(_T)
		, d(_d)
		, trainingset(_trainingset)
		, document_query_summary(create_dqs(d, trainingset))
		, idf(d.features.size())
		, features(d.objects.size(), d.dependencies.size())
		, E_cached(ir_feature_size, d.objects.size())
		, p(T, d.objects.size())
		, h(T, ir_feature_size) // Initialize with non existing feature
		, alpha(T)
	{
		init_idf();

		std::vector<std::pair<feature_id_t, float>> query_row;
		trainingset.citerate([&](auto const& original_row) {
			query_row.clear();

			for (auto const& kvp : original_row) {
				query_row.emplace_back(kvp);
			}

			auto&& row = features[original_row.row_i];
			d.dependencies.keys([&](document_id_t d_id) {
				row[d_id] = compute_features(query_row, d_id);
			});
		});

		ir_feature_id_t::iterate([&](ir_feature_id_t k) {
			std::cout << k.unseal() << std::endl;
			trainingset.citerate([&](auto const& row) {
				object_id_t i = row.row_i;
				E_cached[k][i] = compute_E(create_ranking_weak(i, k), i);
			});
		}, ir_feature_size);

		const float m = d.objects.size();
		trainingset.citerate([&](auto&& row) {
			object_id_t i = row.row_i;
			p[0][i] = 1.0f / m;
		});

		t_t::iterate([&](t_t t) {
			std::cout << "t: " << t.unseal() << std::endl;
			h[t] = compute_h(t);
			std::cout << "h[t]: " << h[t].unseal() << std::endl;
			alpha[t] = compute_alpha(t);
			std::cout << "alpha[t]: " << alpha[t] << std::endl;

			if (t.unseal()+1 >= T) {
				return;
			}

			encapsulated_vector<object_id_t, float> strong_E(d.objects.size());
			float sum_strong_E = 0.0f;
			trainingset.citerate([&](auto&& row) {
				object_id_t i = row.row_i;
				float sei = std::exp(-1.0f * compute_E(create_ranking_strong(i, t), i));
				strong_E[i] = sei;
				sum_strong_E += sei;
			});
			std::cout << "sum strong_E: " << sum_strong_E << std::endl;

			trainingset.citerate([&](auto&& row) {
				object_id_t i = row.row_i;
				p[t_t(t.unseal()+1)][i] = strong_E[i] / sum_strong_E;
			});
		}, T);
	}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& test_row) const
	{
		ranking_t ranking;

		d.dependencies.keys([&](document_id_t d_id) {
			float f = compute_f(T-1, compute_features(test_row, d_id));
			if (f >= 0.0f) {
				ranking.emplace_back(std::make_pair(d_id, f));
			}
		});

		return ranking;
	}
};

}

