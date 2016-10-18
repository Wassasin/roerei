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
	struct occurance_t
	{
		uri_t uri;
		size_t freq;
		size_t depth;
	};

	std::string corpus;
	std::string file;
	uri_t uri;
	std::vector<occurance_t> type_uris;
	boost::optional<std::vector<occurance_t>> body_uris;
};

}

BOOST_FUSION_ADAPT_STRUCT(
		roerei::summary_t::occurance_t,
		(roerei::uri_t, uri)
		(size_t, freq)
		(size_t, depth)
)

BOOST_FUSION_ADAPT_STRUCT(
		roerei::summary_t,
		(std::string, corpus)
		(std::string, file)
		(roerei::uri_t, uri)
		(std::vector<roerei::summary_t::occurance_t>, type_uris)
		(boost::optional<std::vector<roerei::summary_t::occurance_t>>, body_uris)
)
