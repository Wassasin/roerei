#pragma once

#include <roerei/ml/posetcons_canonical.hpp>
#include <roerei/dataset.hpp>

namespace roerei
{

class sampler {
public:
	static dataset_t sample(dataset_t const& original_d, float fragment = 0.05f)
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
		encapsulated_vector<dependency_id_t, uri_t> dependencies(new_d.dependencies);

		dataset_t::feature_matrix_t feature_matrix(objects.size(), new_d.features.size());
		feature_matrix.iterate([&](dataset_t::feature_matrix_t::row_proxy_t row) {
			for(auto old_kvp : new_set[row.row_i]) {
				row[old_kvp.first] = old_kvp.second;
			}
		});

		dataset_t::dependency_matrix_t dependency_matrix(objects.size(), new_d.dependencies.size());
		dependency_matrix.iterate([&](dataset_t::dependency_matrix_t::row_proxy_t row) {
			for(auto old_kvp : new_d.dependency_matrix[row.row_i]) {
				row[old_kvp.first] = old_kvp.second;
			}
		});

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
