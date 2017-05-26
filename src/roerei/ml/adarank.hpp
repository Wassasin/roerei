#pragma once

#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>
#include <roerei/performance.hpp>

#include <roerei/generic/full_matrix.hpp>
#include <roerei/generic/id_t.hpp>

#include <roerei/util/performance.hpp>
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

	static constexpr size_t ir_feature_size = 1;

	typedef std::vector<std::pair<dependency_id_t, float>> ranking_t;

private:
	size_t T;
	dataset_t const& d;
	MATRIX const& trainingset;

	sparse_matrix_t<dependency_id_t, ir_feature_id_t, float> x;
	//sparse_matrix_t<feature_id_t, dependency_id_t, float> x_rev;

	full_matrix_t<ir_feature_id_t, object_id_t, float> E_cached;
	full_matrix_t<t_t, object_id_t, float> p;

	encapsulated_vector<t_t, ir_feature_id_t> h;
	encapsulated_vector<t_t, float> alpha;

private:
	void init_features()
	{
		/*auto dependants = dependencies::create_dependants(d);
		d.dependencies.keys([&](dependency_id_t dep_id) {
			for (object_id_t obj_id : dependants[dep_id]) {
				try {
					for (std::pair<feature_id_t, float> kvp : trainingset[obj_id]) {
						x[dep_id][kvp.first] += kvp.second;
					}
				} catch (std::out_of_range) {
					// Do nothing
				}
			}
		});*/
	}

	/*void init_features_rev()
	{
		x.citerate([&](auto const& row) {
			dependency_id_t dep_id = row.row_i;
			for (std::pair<feature_id_t, float> kvp : row) {
				x_rev[kvp.first][dep_id] = kvp.second;
			}
		});
	}*/

	float compute_E(ranking_t const& ranking, object_id_t test_row_i)
	{
		return performance::measure_oocover(d, test_row_i, ranking); // Metric we want to maximize
	}

	ranking_t create_ranking_weak(object_id_t i, ir_feature_id_t k)
	{
		ranking_t ranking;
		for (auto&& kvp : d.dependency_matrix[i]) {
			ranking.emplace_back(std::make_pair(kvp.first, x[kvp.first][k]));
		}
		return ranking;
	}

	ranking_t create_ranking_strong(object_id_t query_id, t_t t)
	{
		ranking_t ranking;
		for (auto kvp : d.dependency_matrix[query_id]) {
			ranking.emplace_back(std::make_pair(kvp.first, compute_f(t, x[kvp.first])));
		}
		return ranking;
	}

	ir_feature_id_t compute_h(t_t t)
	{
		ir_feature_id_t result_k = ir_feature_size; // Non-existing feature
		float result_score = std::numeric_limits<float>::lowest();

		ir_feature_id_t::iterate([&](ir_feature_id_t k) {
			float sum  = 0;
			trainingset.citerate([&](typename MATRIX::const_row_proxy_t const& row) {
				const object_id_t i = row.row_i;
				sum += p[t][i] * E_cached[k][i];
			});

			if (result_score < sum) {
				result_score = sum;
				result_k = k;
			}
		}, ir_feature_size);

		return result_k;
	}

	float compute_alpha(t_t t)
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

	template<typename FEATURES>
	float compute_f(t_t t, FEATURES const& features)
	{
		float result = 0.0f;
		t_t::iterate([&](t_t k) {
			result += alpha[k] * features[h[k]];
		}, t.unseal()+1);
		return result;
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
		, x(d.dependencies.size(), ir_feature_size)
		//, x_rev(d.features.size(), d.dependencies.size())
		, E_cached(ir_feature_size, d.objects.size())
		, p(T, d.objects.size())
		, h(T, ir_feature_size) // Initialize with non existing feature
		, alpha(T)
	{
		init_features();
		//init_features_rev();

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

			encapsulated_vector<object_id_t, float> strong_E(d.objects.size());
			float sum_strong_E = 0.0f;
			trainingset.citerate([&](auto&& row) {
				object_id_t i = row.row_i;
				float sei = std::exp(-1.0f * compute_E(create_ranking_strong(i, t), i));
				strong_E[i] = sei;
				sum_strong_E += sei;
			});
			std::cout << "sum strong_E: " << sum_strong_E << std::endl;

			if (t.unseal()+1 >= T) {
				return;
			}

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
		d.dependencies.keys([&](dependency_id_t dep_id) {

		});
		/*for (auto kvp : d.dependency_matrix[query_id]) {
			ranking.emplace_back(std::make_pair(kvp.first, compute_f(t, x[kvp.first])));
		}
		return ranking;*/

		//normalize::exec(ranks);
		return {};
	}
};

}

