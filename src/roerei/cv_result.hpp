#pragma once

#include <roerei/performance.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

namespace roerei
{

struct cv_result_t
{
	std::string ml;
	size_t n, k;
	performance::metrics_t metrics;
};

}

BOOST_FUSION_ADAPT_STRUCT(
		roerei::cv_result_t,
		(std::string, ml)
		(size_t, n)
		(size_t, k)
		(roerei::performance::metrics_t, metrics)
)
