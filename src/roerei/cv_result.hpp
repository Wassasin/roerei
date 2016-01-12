#pragma once

#include <roerei/performance.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

namespace roerei
{

struct knn_params_t
{
	size_t k;

	bool operator<(knn_params_t const rhs) const
	{
		return k < rhs.k;
	}
};

struct nb_params_t
{
	float pi, sigma, tau;

	bool operator<(nb_params_t const rhs) const
	{
		return pi < rhs.pi || sigma < rhs.sigma || tau < rhs.tau;
	}
};

struct cv_result_t
{
	std::string corpus;
	std::string strat;
	std::string ml;
	boost::optional<knn_params_t> knn_params;
	boost::optional<nb_params_t> nb_params;
	size_t n, k;
	performance::metrics_t metrics;
};

std::ostream& operator<<(std::ostream& os, cv_result_t const& rhs)
{
	if(rhs.ml == "knn")
		return os << rhs.corpus << ": [" << rhs.strat << "] " << rhs.ml << " K=" << rhs.knn_params->k << " " << rhs.metrics;

	if(rhs.ml == "nb")
		return os << rhs.corpus << ": [" << rhs.strat << "] " << rhs.ml << " " << rhs.nb_params->pi << " " << rhs.nb_params->sigma << " " << rhs.nb_params->tau << " " << rhs.metrics;
}

}

BOOST_FUSION_ADAPT_STRUCT(
		roerei::knn_params_t,
		(size_t, k)
)

BOOST_FUSION_ADAPT_STRUCT(
		roerei::nb_params_t,
		(float, pi)
		(float, sigma)
		(float, tau)
)

BOOST_FUSION_ADAPT_STRUCT(
		roerei::cv_result_t,
		(std::string, corpus)
		(std::string, strat)
		(std::string, ml)
		(boost::optional<roerei::knn_params_t>, knn_params)
		(boost::optional<roerei::nb_params_t>, nb_params)
		(size_t, n)
		(size_t, k)
		(roerei::performance::metrics_t, metrics)
)
