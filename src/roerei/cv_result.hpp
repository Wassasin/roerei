#pragma once

#include <roerei/performance.hpp>
#include <roerei/ml/ml_type.hpp>
#include <roerei/ml/posetcons_type.hpp>

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

  bool operator==(nb_params_t const rhs) const
  {
    return std::tie(pi, sigma, tau) == std::tie(rhs.pi, rhs.sigma, rhs.tau);
  }

  bool operator<(nb_params_t const rhs) const
  {
    return std::tie(pi, sigma, tau) < std::tie(rhs.pi, rhs.sigma, rhs.tau);
  }
};

struct adarank_params_t
{
	size_t T;

	bool operator<(adarank_params_t const rhs) const
	{
	  return T < rhs.T;
	}
};


struct cv_result_t
{
  std::string corpus;
  posetcons_type strat;
  ml_type ml;
  boost::optional<knn_params_t> knn_params;
  boost::optional<nb_params_t> nb_params;
  boost::optional<adarank_params_t> adarank_params;
  size_t n, k;
  performance::metrics_t metrics;
};

inline std::ostream& operator<<(std::ostream& os, cv_result_t const& rhs)
{
  os << rhs.corpus << ": [" << rhs.strat << "] " << rhs.ml << " ";
  switch(rhs.ml)
  {
  case ml_type::knn:
    os << "K=" << rhs.knn_params->k << " ";
    break;
  case ml_type::knn_adaptive:
  case ml_type::omniscient:
  case ml_type::ensemble:
	break;
  case ml_type::adarank:
	os << "T=" << rhs.adarank_params->T;
    break;
  case ml_type::naive_bayes:
    os << rhs.nb_params->pi << " " << rhs.nb_params->sigma << " " << rhs.nb_params->tau << " ";
    break;
  }
  return os << " " << rhs.metrics;
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
	roerei::adarank_params_t,
	(size_t, T)
)

BOOST_FUSION_ADAPT_STRUCT(
    roerei::cv_result_t,
    (std::string, corpus)
    (roerei::posetcons_type, strat)
    (roerei::ml_type, ml)
    (boost::optional<roerei::knn_params_t>, knn_params)
    (boost::optional<roerei::nb_params_t>, nb_params)
	(boost::optional<roerei::adarank_params_t>, adarank_params)
    (size_t, n)
    (size_t, k)
    (roerei::performance::metrics_t, metrics)
)
