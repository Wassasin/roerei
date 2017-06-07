#pragma once

#include <roerei/ml/posetcons_canonical.hpp>
#include <roerei/dataset.hpp>

namespace roerei
{

class sampler {
public:
	static dataset_t sample(dataset_t const& original_d, float fragment = 0.001f)
	{
		auto const new_d = posetcons_canonical::consistentize(original_d);
		size_t const new_length = new_d.objects.size() * fragment;

		auto const new_set = posetcons_canonical::exec(new_d.feature_matrix, new_d.feature_matrix[object_id_t(new_length)]);

		encapsulated_vector<object_id_t, uri_t> objects;
		objects.reserve(new_length);
		object_id_t::iterate([&](object_id_t i) {
			objects.emplace_back(new_d.objects[i]);
		}, new_length);

		encapsulated_vector<feature_id_t, uri_t> features(new_d.features);

		dataset_t::feature_matrix_t feature_matrix(objects.size(), new_d.features.size());
		feature_matrix.iterate([&](dataset_t::feature_matrix_t::row_proxy_t row) {
			for(auto old_kvp : new_set[row.row_i]) {
				row[old_kvp.first] = old_kvp.second;
			}
		});

		std::set<dependency_id_t> used_deps;
		object_id_t::iterate([&](object_id_t i) {
			for(auto old_kvp : new_d.dependency_matrix[i]) {
				if (old_kvp.second > 0) {
					used_deps.emplace(old_kvp.first);
				}
			}
		}, new_length);

		// new -> old
		encapsulated_vector<dependency_id_t, dependency_id_t> dep_map;
		dep_map.reserve(used_deps.size());
		for(dependency_id_t old_id : used_deps) {
			dep_map.emplace_back(old_id);
		}

		dataset_t::dependency_matrix_t dependency_matrix(objects.size(), dep_map.size());
		dependency_matrix.iterate([&](dataset_t::dependency_matrix_t::row_proxy_t row) {
			for(auto old_kvp : new_d.dependency_matrix[row.row_i]) {
				row[dep_map[old_kvp.first]] = old_kvp.second;
			}
		});

		encapsulated_vector<dependency_id_t, uri_t> dependencies;
		dependencies.reserve(dep_map.size());
		for (dependency_id_t old_id : dep_map) {
			dependencies.emplace_back(new_d.dependencies[old_id]);
		}

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
