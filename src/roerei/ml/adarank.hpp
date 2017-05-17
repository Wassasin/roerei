#pragma once

#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>

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
	size_t D;
	dataset_t const& d;
	MATRIX const& trainingset;

public:
	adarank(
			size_t _D,
			dataset_t const& _d,
			MATRIX const& _trainingset
			)
		: D(_D)
		, d(_d)
		, trainingset(_trainingset)
	{
	}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& test_row) const
	{
		//normalize::exec(ranks);
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
