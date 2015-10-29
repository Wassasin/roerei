#pragma once

#include <roerei/sparse_unit_matrix.hpp>
#include <roerei/dataset.hpp>

#include <roerei/create_map.hpp>

namespace roerei
{

class dependencies
{
	dependencies() = delete;

	template<typename MATRIX, typename F>
	static void _iterate_dependants_helper(MATRIX const& dependants, size_t i, F const& yield, std::set<size_t>& visited)
	{
		if(!visited.emplace(i).second)
			return;

		yield(i);
		for(size_t const j : dependants[i])
			_iterate_dependants_helper(dependants, j, yield, visited);
	}

public:
	static sparse_unit_matrix_t create_dependants(dataset_t const& d)
	{
		std::map<uri_t, size_t> objects_map(create_map<uri_t>(d.objects));
		sparse_unit_matrix_t result(d.objects.size(), d.objects.size());

		d.dependency_matrix.citerate([&](dataset_t::matrix_t::const_row_proxy_t const& xs) {
			for(auto const& kvp : xs)
			{
				size_t dep_i = objects_map[d.dependencies[kvp.first]];

				if(kvp.second > 0)
					result.set(std::make_pair(dep_i, xs.row_i));
			}
		});
		return result;
	}

	template<typename MATRIX, typename F>
	static void iterate_dependants(MATRIX const& dependants, size_t i, F const& yield)
	{
		std::set<size_t> visited;
		_iterate_dependants_helper(dependants, i, yield, visited);
	}
};

}
