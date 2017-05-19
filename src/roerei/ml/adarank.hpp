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

	typedef std::vector<std::pair<dependency_id_t, float>> ranking_t;

private:
	size_t T;
	dataset_t const& d;
	MATRIX const& trainingset;

	full_matrix_t<t_t, object_id_t, float> p;
	full_matrix_t<dependency_id_t, feature_id_t, float> x;

	encapsulated_vector<t_t, feature_id_t> h;
	encapsulated_vector<t_t, float> alpha;

private:
	static decltype(x) create_features(dataset_t const& d)
	{
		full_matrix_t<dependency_id_t, feature_id_t, float> x(d.dependencies.size(), d.features.size());

		auto dependants = dependencies::create_dependants(d);
		d.dependencies.keys([&](dependency_id_t dep_id) {
			for (object_id_t obj_id : dependants[dep_id]) {
				for (std::pair<feature_id_t, float> kvp : d.feature_matrix[obj_id]) {
					x[dep_id][kvp.first] += kvp.second;
				}
			}
		});

		return x;
	}

	float E(ranking_t const& ranking, object_id_t test_row_i)
	{
		return performance::measure(d, test_row_i, ranking).metrics.oocover; // Metric we want to maximize
	}

	ranking_t create_ranking(feature_id_t k)
	{
		ranking_t ranking;
		d.dependencies.keys([&](dependency_id_t doc) {
			ranking.emplace_back(std::make_pair(doc, x[doc][k]));
		});
		return ranking;
	}

	feature_id_t compute_h(t_t t)
	{
		feature_id_t result_k = d.features.size(); // Non-existing feature
		float result_score = std::numeric_limits<float>::min();

		d.features.keys([&](feature_id_t k) {
			std::cout << k.unseal() << std::endl;
			float sum  = 0;
			ranking_t ranking = create_ranking(k);
			trainingset.citerate([&](typename MATRIX::const_row_proxy_t const& row) {
				const object_id_t i = row.row_i;
				sum += p[t][i] * E(ranking, i);
			});

			if (result_score < sum) {
				result_score = sum;
				result_k = k;
			}
		});

		return result_k;
	}

	float compute_alpha(t_t t)
	{
		ranking_t ranking = create_ranking(h[t]);

		float a = 0.0f;
		float b = 0.0f;

		trainingset.citerate([&](auto const& row) {
			object_id_t i = row.row_i;
			float e = E(ranking, i);

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
		, p(T, d.objects.size())
		, x(adarank::create_features(d)) // TODO Only consider training set
		, h(T, d.features.size()) // Initialize with non existing feature
		, alpha(T)
	{
		auto row = p[0];
		const float m = d.objects.size();
		for (auto& pij : row) {
			pij = 1.0f / m;
		}

		t_t::iterate([&](t_t t) {
			h[t] = compute_h(t);
			std::cout << "h[t]: " << h[t].unseal() << std::endl;
			alpha[t] = compute_alpha(t);
			std::cout << "alpha[t]: " << alpha[t] << std::endl;


			std::cout << "t: " << t.unseal() << std::endl;
		}, T);
	}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& /*test_row*/) const
	{
		//normalize::exec(ranks);
		return {};
	}
};

}

