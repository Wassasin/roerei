#pragma once

#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>
#include <roerei/performance.hpp>

#include <roerei/generic/full_matrix.hpp>
#include <roerei/generic/id_t.hpp>

#include <roerei/util/performance.hpp>

#include <algorithm>
#include <cstdint>

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

private:
	size_t T;
	dataset_t const& d;
	MATRIX const& trainingset;

	full_matrix_t<t_t, object_id_t, float> p;
	full_matrix_t<dependency_id_t, feature_id_t, float> x;



private:
	static decltype(x) create_features(dataset_t const& d)
	{
		full_matrix_t<dependency_id_t, feature_id_t, float> x(d.dependencies.size(), d.features.size());

		auto dependants = dependencies::create_dependants(d);
		d.dependencies.keys([&](dependency_id_t dep_id) {
			for (object_id_t obj_id : dependants[dep_id]) {
				for (std::pair<feature_id_t, float> kvp : d.feature_matrix[obj_id]) {
					x[dep_id][kvp.first] += kvp.second;
					std::cout << x[dep_id][kvp.first] << std::endl;
				}
			}
		});

		return x;
	}

	float E(std::pair<dependency_id_t, float> pi, object_id_t test_row_i)
	{
		return performance::measure(d, test_row_i, pi).metrics.oocover; // Metric we want to maximize
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
	{
		auto row = p[0];
		const float m = d.objects.size();
		for (auto& pij : row) {
			pij = 1.0f / m;
		}
	}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& /*test_row*/) const
	{
		//normalize::exec(ranks);
		return {};
	}
};

}

/*
struct d_t : public id_t<d_t>
{
	d_t(size_t _id) : id_t<d_t>(_id) {}
};

struct k_t : public id_t<k_t>
{
	k_t(size_t _id) : id_t<k_t>(_id) {}
};


enum class valuation_t : std::int8_t {
	negative = -1,
	positive = 1,
};

typedef std::pair<dependency_id_t, valuation_t> pair_t;
typedef std::vector<pair_t> subset_t;

	full_matrix_t<d_t, k_t, float> alpha, beta;

	float alphaf(subset_t const& c)
	{
		float n_neg = std::count()
	}

	float betaf(subset_t const& c)
	{

	}

	float lambda(d_t d, k_t kn, subset_t const& c)
	{
		k_t k(kn.unseal()-1);

		return (alpha[d][kn] - alpha[d][k]) * betaf(c) - (beta[d][kn] - beta[d][k]) * alphaf(c);
	}*/
