#pragma once

#include <roerei/uri.hpp>

#include <vector>
#include <type_traits>
#include <boost/optional.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

namespace roerei
{

struct summary_t
{
	struct frequency_t
	{
		uri_t uri;
		size_t freq;
	};

	std::string file;
	uri_t uri;
	std::vector<frequency_t> type_uris;
	boost::optional<std::vector<frequency_t>> body_uris;
};

}

BOOST_FUSION_ADAPT_STRUCT(
		roerei::summary_t::frequency_t,
		(roerei::uri_t, uri)
		(size_t, freq)
)

BOOST_FUSION_ADAPT_STRUCT(
		roerei::summary_t,
		(std::string, file)
		(roerei::uri_t, uri)
		(std::vector<roerei::summary_t::frequency_t>, type_uris)
		(boost::optional<std::vector<roerei::summary_t::frequency_t>>, body_uris)
)
