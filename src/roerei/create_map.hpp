#pragma once

#include <roerei/encapsulated_vector.hpp>

#include <map>

namespace roerei
{

template<typename ID, typename T>
std::map<T, ID> create_map(encapsulated_vector<ID, T> const& xs)
{
	std::map<T, ID> result;
	xs.iterate([&](ID id, T const& x) {
		result.emplace(std::make_pair(x, id));
	});
	return result;
}

}
