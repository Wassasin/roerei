#pragma once

#include <map>

namespace roerei
{

template<typename A, typename CONTAINER>
std::map<A, size_t> create_map(CONTAINER const& xs)
{
	std::map<A, size_t> result;

	size_t i = 0;
	for(auto const& x : xs)
		result.emplace(std::make_pair(x, i++));

	return result;
}

}
