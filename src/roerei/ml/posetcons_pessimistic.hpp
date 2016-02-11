#pragma once

#include <roerei/dataset.hpp>

#include <roerei/generic/encapsulated_vector.hpp>
#include <roerei/generic/bl_sparse_matrix.hpp>

#include <map>
#include <set>

namespace roerei
{

class posetcons_pessimistic
{
private:
	encapsulated_vector<object_id_t, std::vector<object_id_t>> dependants_real;

	static decltype(dependants_real) generate_dependants(dataset_t const& d)
	{
		std::map<dependency_id_t, object_id_t> dependency_map(d.create_dependency_map());
		dependencies::dependant_obj_matrix_t dependants_trans(dependencies::create_obj_dependants(d, dependency_map));
		dependants_trans.transitive();

		decltype(dependants_real) dependants_real(dependants_trans.size_m());
		d.objects.keys([&](object_id_t i) {
			std::set<object_id_t> const& objs = dependants_trans[i];
			dependants_real[i].insert(dependants_real[i].end(), objs.begin(), objs.end());
		});
		return dependants_real;
	}

public:
	posetcons_pessimistic(dataset_t const& d)
		: dependants_real(generate_dependants(d))
	{}

	template<typename TRAINSET>
	bl_sparse_matrix_t<TRAINSET> exec(TRAINSET const& train_m, object_id_t test_row_i) const
	{
		return bl_sparse_matrix_t<TRAINSET>(train_m, dependants_real[test_row_i]);
	}
};

}
