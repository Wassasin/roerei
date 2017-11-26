#pragma once

#include <roerei/dataset.hpp>

#include <roerei/generic/encapsulated_vector.hpp>
#include <roerei/generic/wl_sparse_matrix.hpp>

#include <map>
#include <set>

namespace roerei
{

class posetcons_optimistic
{
private:
	encapsulated_vector<object_id_t, std::vector<object_id_t>> parents_real;

	static decltype(parents_real) generate_parents(dataset_t const& d)
	{
		dependencies::dependant_obj_matrix_t parents_trans(dependencies::create_obj_dependants(d).transpose());
		parents_trans.transitive();

		decltype(parents_real) parents_real(parents_trans.size_m());
		d.objects.keys([&](object_id_t i) {
			parents_trans.citerate(i, [&](object_id_t const j) {
				parents_real[i].emplace_back(j);
			});
		});
		return parents_real;
	}

public:
	posetcons_optimistic(dataset_t const& d)
		: parents_real(generate_parents(d))
	{}

	template<typename TRAINSET>
	wl_sparse_matrix_t<TRAINSET> exec(TRAINSET const& train_m, object_id_t test_row_id) const
	{
		return wl_sparse_matrix_t<TRAINSET>(train_m, parents_real[test_row_id]);
	}
};

}
