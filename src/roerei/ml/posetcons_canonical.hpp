#pragma once

#include <roerei/dependencies.hpp>
#include <roerei/dataset.hpp>
#include <roerei/generic/split_sparse_matrix.hpp>

#include <algorithm>
#include <random>

namespace roerei
{

class posetcons_canonical
{
public:
	static dataset_t consistentize(dataset_t const& d, boost::optional<uint64_t> /*seed*/ = boost::none)
	{
		auto dependants(dependencies::create_obj_dependants(d));
		dependants.transitive();

		for(size_t i = 0; i < d.objects.size(); ++i) {
			dependants.set(std::make_pair(i, i), false); // Remove axioms
		}

		// new -> old
		encapsulated_vector<object_id_t, object_id_t> objs_ordered;
		objs_ordered.reserve(d.objects.size());
		dependants.topological_sort([&](object_id_t i) {
			objs_ordered.emplace_back(i);
		});
		std::reverse(objs_ordered.begin(), objs_ordered.end());

		encapsulated_vector<object_id_t, uri_t> objects;
		objects.reserve(d.objects.size());
		objs_ordered.iterate([&](object_id_t /*new_id*/, object_id_t old_id) {
			objects.emplace_back(d.objects[old_id]);
		});
		encapsulated_vector<feature_id_t, uri_t> features(d.features);
		encapsulated_vector<dependency_id_t, uri_t> dependencies(d.dependencies);

		dataset_t::feature_matrix_t feature_matrix(d.objects.size(), d.features.size());
		feature_matrix.iterate([&](dataset_t::feature_matrix_t::row_proxy_t row) {
			object_id_t old_id = objs_ordered[row.row_i];
			for(auto old_kvp : d.feature_matrix[old_id])
				row[old_kvp.first] = old_kvp.second;
		});

		dataset_t::dependency_matrix_t dependency_matrix(d.objects.size(), d.dependencies.size());
		dependency_matrix.iterate([&](dataset_t::dependency_matrix_t::row_proxy_t row) {
			object_id_t old_id = objs_ordered[row.row_i];
			for(auto old_kvp : d.dependency_matrix[old_id])
				row[old_kvp.first] = old_kvp.second;
		});

		std::set<object_id_t> prior_objects;
		objs_ordered.iterate([&](object_id_t new_id, object_id_t old_id) {
			if (d.prior_objects.find(old_id) != d.prior_objects.end()) {
				prior_objects.emplace(new_id);
			}
		});

		// Construct new dataset
		return dataset_t(
			std::move(objects),
			std::move(features),
			std::move(dependencies),
			std::move(feature_matrix),
			std::move(dependency_matrix),
			std::move(prior_objects)
		);
	}

	template<typename TRAINSET, typename TESTROW>
	static split_sparse_matrix_t<TRAINSET> exec(TRAINSET const& train_m, TESTROW const& test_row)
	{
		return split_sparse_matrix_t<TRAINSET>(train_m, test_row.row_i);
	}
};

}
