#pragma once

#include <roerei/dependencies.hpp>
#include <roerei/dataset.hpp>

#include <algorithm>

namespace roerei
{

class posetcons_canonical
{
public:
	static dataset_t consistentize(dataset_t const& d, boost::optional<uint64_t> seed = boost::none)
	{
		auto dependants_trans(dependencies::create_obj_dependants(d));
		dependants_trans.transitive();

		encapsulated_vector<object_id_t, object_id_t> objs_ordered;
		objs_ordered.reserve(d.objects.size());
		d.objects.keys([&objs_ordered](object_id_t x) {
			objs_ordered.emplace_back(x);
		});

		if(seed) /* Seed for the linearization of dependants_trans */
		{
			std::mt19937 g(*seed);
			std::shuffle(objs_ordered.begin(), objs_ordered.end(), g);
		}

		std::stable_sort(objs_ordered.begin(), objs_ordered.end(), [&dependants_trans](object_id_t x, object_id_t y) {
			return dependants_trans[std::make_pair(x, y)];
		});

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

		// Construct new dataset
		return dataset_t(
			std::move(objects),
			std::move(features),
			std::move(dependencies),
			std::move(feature_matrix),
			std::move(dependency_matrix)
		);
	}
};

}
