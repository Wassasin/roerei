#pragma once

#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>
#include <roerei/performance.hpp>

#include <roerei/ml/ml_type.hpp>

#include <roerei/ml/knn.hpp>
#include <roerei/ml/knn_adaptive.hpp>
#include <roerei/ml/omniscient.hpp>
#include <roerei/ml/naive_bayes.hpp>
#include <roerei/ml/adarank.hpp>

#include <roerei/ml/posetcons_canonical.hpp>
#include <roerei/ml/posetcons_pessimistic.hpp>

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
	static void iterate_all(ml_type method, dataset_t const& d_orig, boost::optional<std::string> filter = boost::none)
	{
		auto const d(posetcons_canonical::consistentize(d_orig));

		posetcons_pessimistic pc(d);

		d.objects.iterate([&](object_id_t i, uri_t const& uri) {
			if(filter && uri.find(*filter) == std::string::npos)
				return;

			auto feature_matrix(pc.exec(d.feature_matrix, i));
			std::shared_ptr<nb_preload_data_t> nb_data;

			performance::result_t result = ([&]() {
				switch(method)
				{
				case ml_type::knn:
				{
					knn<decltype(feature_matrix)> ml(5, feature_matrix, d);
					return performance::measure(d, i, ml.predict(d.feature_matrix[i]));
				}
				case ml_type::knn_adaptive:
				{
					knn_adaptive<decltype(feature_matrix)> ml(feature_matrix, d);
					return performance::measure(d, i, ml.predict(d.feature_matrix[i]));
				}
				case ml_type::omniscient:
				{
					omniscient ml(d);
					return performance::measure(d, i, ml.predict(d.feature_matrix[i]));
				}
				case ml_type::naive_bayes:
				{
					if(!nb_data)
						nb_data = std::make_shared<nb_preload_data_t>(d);

					naive_bayes<decltype(feature_matrix)> ml(
						10, -15, 0,
						d,
						*nb_data,
						feature_matrix
					);
					return performance::measure(d, i, ml.predict(d.feature_matrix[i], i));
				}
				case ml_type::adarank:
				{
					adarank<decltype(feature_matrix)> ml(5, d, feature_matrix);
					return performance::measure(d, i, ml.predict(d.feature_matrix[i]));
				}
				case ml_type::ensemble:
					throw std::runtime_error("Not implemented ensemble");
				}
			})();

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
