#pragma once

#include <roerei/dataset.hpp>
#include <roerei/distance.hpp>

#include <list>
#include <algorithm>

namespace roerei
{

template<typename MATRIX>
class knn_adaptive
{
private:
	typedef std::pair<object_id_t, float> distance_t;

private:
	MATRIX const& trainingset;
	dataset_t const& d;

public:
	knn_adaptive(MATRIX const& _trainingset, dataset_t const& _d)
		: trainingset(_trainingset)
		, d(_d)
	{}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& ys) const
	{
		std::vector<distance_t> items;

		trainingset.citerate([&](typename MATRIX::const_row_proxy_t const& xs) {
			float d = distance::euclidean<decltype(xs.begin()->second), decltype(xs), ROW>(xs, ys);
			items.emplace_back(std::make_pair(xs.row_i, d));
		});

		std::sort(items.begin(), items.end(), [](distance_t const& x, distance_t const& y) {
			return x.second < y.second;
		});

		std::map<dependency_id_t, float> suggestions;
		for(distance_t const& kvp : items)
		{
			float weight = 1.0f / (kvp.second + 1.0f); // "Similarity", higher is more similar

			for(auto dep_kvp : d.dependency_matrix[kvp.first])
				suggestions[dep_kvp.first] += ((float)dep_kvp.second) * weight;

			if(suggestions.size() >= 1024)
				break;
		}

		return std::vector<std::pair<dependency_id_t, float>>(suggestions.begin(), suggestions.end());
	}
};

}
