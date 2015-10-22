#pragma once

#include <roerei/dataset.hpp>

#include <algorithm>
#include <list>
#include <map>
#include <set>

namespace roerei
{

class performance
{
private:
	performance() = delete;

public:
	struct result_t
	{
		float oocover, ooprecision;
		std::list<std::pair<size_t, float>> predictions;
		std::vector<std::pair<size_t, float>> suggestions_sorted;
		std::set<size_t> required_deps, suggested_deps, found_deps, missing_deps;
	};

	template<typename ML, typename ROW>
	static result_t measure(dataset_t const& d, ML const& ml, ROW const& test_row)
	{
		std::map<size_t, float> suggestions; // <id, weighted freq>

		auto predictions(ml.predict(test_row));
		std::reverse(predictions.begin(), predictions.end());
		for(auto const& kvp : predictions)
		{
			float weight = 1.0f / (kvp.second + 1.0f); // "Similarity", higher is more similar

			for(auto dep_kvp : d.dependency_matrix[kvp.first])
				suggestions[dep_kvp.first] += ((float)dep_kvp.second) * weight;
		}

		std::vector<std::pair<size_t, float>> suggestions_sorted;
		for(auto const& kvp : suggestions)
			suggestions_sorted.emplace_back(kvp);

		std::sort(suggestions_sorted.begin(), suggestions_sorted.end(), [&](std::pair<size_t, float> const& x, std::pair<size_t, float> const& y) {
			return x.second > y.second;
		});

		std::set<size_t> required_deps, suggested_deps, found_deps, missing_deps;
		for(auto const& kvp : d.dependency_matrix[test_row.row_i])
			required_deps.insert(kvp.first);

		for(size_t j = 0; j < suggestions_sorted.size() && j < 100; ++j)
			suggested_deps.insert(suggestions_sorted[j].first);

		std::set_intersection(
			required_deps.begin(), required_deps.end(),
			suggested_deps.begin(), suggested_deps.end(),
			std::inserter(found_deps, found_deps.begin())
		);

		std::set_difference(
			required_deps.begin(), required_deps.end(),
			found_deps.begin(), found_deps.end(),
			std::inserter(missing_deps, missing_deps.begin())
		);

		float c_required = required_deps.size();
		float c_suggested = suggested_deps.size();
		float c_found = found_deps.size();

		float oocover = c_found/c_required;
		float ooprecision = c_found/c_suggested;

		if(required_deps.empty())
		{
			oocover = 1.0f;
			ooprecision = 1.0f;
		}

		return {
			oocover,
			ooprecision,
			std::move(predictions),
			std::move(suggestions_sorted),
			std::move(required_deps),
			std::move(suggested_deps),
			std::move(found_deps),
			std::move(missing_deps)
		};
	}
};

}
