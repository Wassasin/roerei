#pragma once

#include <roerei/sparse_unit_matrix.hpp>
#include <roerei/dataset.hpp>

#include <roerei/create_map.hpp>

namespace roerei
{

class dependencies
{
	dependencies() = delete;

public:
	typedef sparse_unit_matrix_t<dependency_id_t, object_id_t> dependant_matrix_t;
	typedef sparse_unit_matrix_t<object_id_t, object_id_t> dependant_obj_matrix_t;

private:
	template<typename F>
	static void _iterate_dependants_helper(dependant_obj_matrix_t const& dependants, object_id_t i, F const& yield, std::set<object_id_t>& visited)
	{
		if(!visited.emplace(i).second)
			return;

		yield(i);
		for(object_id_t const j : dependants[i])
			_iterate_dependants_helper(dependants, j, yield, visited);
	}

public:
	static dependant_matrix_t create_dependants(dataset_t const& d)
	{
		dependant_matrix_t result(d.dependencies.size(), d.objects.size());
		d.dependency_matrix.citerate([&](dataset_t::dependency_matrix_t::const_row_proxy_t const& xs) {
			for(auto const& kvp : xs)
			{
				if(kvp.second > 0)
					result.set(std::make_pair(kvp.first, xs.row_i));
			}
		});
		return result;
	}

	static dependant_obj_matrix_t create_obj_dependants(dataset_t const& d, std::map<dependency_id_t, object_id_t> const& dependency_map)
	{
		dependant_obj_matrix_t dependants_objs(d.objects.size(), d.objects.size());
		d.dependency_matrix.citerate([&](dataset_t::dependency_matrix_t::const_row_proxy_t const& xs) {
			for(auto const& kvp : xs)
			{
				if(kvp.second > 0)
				{
					auto it = dependency_map.find(kvp.first);
					if(it == dependency_map.end())
						return;
					dependants_objs.set(std::make_pair(it->second, xs.row_i));
				}
			}
		});

		return dependants_objs;
	}

	static dependant_obj_matrix_t create_obj_dependants(dataset_t const& d)
	{
		return create_obj_dependants(d, d.create_dependency_map());
	}

	template<typename F>
	static void iterate_dependants(dependant_obj_matrix_t const& dependants, object_id_t i, F const& yield)
	{
		std::set<object_id_t> visited;
		_iterate_dependants_helper(dependants, i, yield, visited);
	}
};

}
