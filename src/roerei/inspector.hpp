#pragma once

#include <roerei/dataset.hpp>

#include <roerei/ml/knn.hpp>

#include <iostream>

namespace roerei
{

class inspector
{
private:
	inspector() = delete;

	template<typename ROW>
	static void print(ROW const& row)
	{
		std::cout << "[";
		for(auto const& kvp : row)
			std::cout << " " << kvp.second << '*' << kvp.first.unseal();
		std::cout << " ]";
	}

public:
	static void iterate_all(dataset_t const& d)
	{
		auto const dependants(dependencies::create_obj_dependants(d));
		d.objects.keys([&](object_id_t i) {
			sliced_sparse_matrix_t<decltype(d.feature_matrix) const> feature_matrix(d.feature_matrix, true);
			dependencies::iterate_dependants(dependants, i, [&](object_id_t j)
			{
				feature_matrix.try_remove_key(j);
			});

			knn<decltype(feature_matrix)> c(5, feature_matrix, d);
			performance::result_t result(performance::measure(d, i, c.predict(d.feature_matrix[i])));

			std::cout << i.unseal() << " " << d.objects[i] << " ";
			print(d.feature_matrix[i]);
			std::cout << std::endl;

			std::cout << "-- Types" << std::endl;
			for(auto const& kvp : d.feature_matrix[i])
				std::cout << kvp.second << "*" << kvp.first.unseal() << " " << d.features[kvp.first] << std::endl;

			std::cout << "-- Required" << std::endl;
			{
				bool empty = true;
				for(auto const& kvp : d.dependency_matrix[i])
				{
					empty = false;
					std::cout << kvp.second << "*" << kvp.first.unseal() << " " << d.dependencies[kvp.first] << std::endl;
				}
				if(empty)
					std::cout << "Empty set" << std::endl;
			}

			std::cout << "-- Suggestions" << std::endl;
			for(size_t j = 0; j < result.suggestions_sorted.size(); j++)
			{
				auto const& kvp = result.suggestions_sorted[j];
				if(std::binary_search(result.required_deps.begin(), result.required_deps.end(), kvp.first))
					std::cout << "! ";

				std::cout << kvp.second << " <= " << kvp.first.unseal() << " " << d.dependencies[kvp.first] << std::endl;
			}

			std::cout << "-- Missing" << std::endl;
			{
				std::set<dependency_id_t> oomissing_deps;
				std::set_difference(
					result.required_deps.begin(), result.required_deps.end(),
					result.oofound_deps.begin(), result.oofound_deps.end(),
					std::inserter(oomissing_deps, oomissing_deps.begin())
				);

				bool empty = true;
				for(dependency_id_t j : oomissing_deps)
				{
					empty = false;
					std::cout << d.dependency_matrix[i][j] << "*" << j.unseal() << " " << d.dependencies[j] << std::endl;
				}
				if(empty)
					std::cout << "Empty set" << std::endl;
			}

			std::cout << "-- Metrics" << std::endl;
			std::cout << "100Cover: " << result.metrics.oocover << std::endl;
			std::cout << "100Precision: " << result.metrics.ooprecision << std::endl;
			std::cout << "Recall: " << result.metrics.recall << std::endl;
			std::cout << "Rank: " << result.metrics.rank << std::endl;
			std::cout << "AUC: " << result.metrics.auc << std::endl;

			std::cout << std::endl;
		});
	}
};

}
