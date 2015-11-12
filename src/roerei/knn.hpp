#pragma once

#include <roerei/dataset.hpp>
#include <roerei/distance.hpp>

#include <list>
#include <algorithm>

namespace roerei
{

template<typename MATRIX>
class knn
{
private:
	typedef std::pair<object_id_t, float> distance_t;

	struct best_set_t
	{
		size_t k;
		std::vector<distance_t> items;

		static inline bool comp(distance_t const& x, distance_t const& y)
		{
			return x.second < y.second;
		}

		best_set_t(size_t _k)
			: k(_k)
			, items()
		{
			items.reserve(k+1);
		}

		void try_add(distance_t&& s);
	};

private:
	size_t k;
	MATRIX const& trainingset;
	dataset_t const& d;

public:
	knn(size_t const _k, MATRIX const& _trainingset, dataset_t const& _d)
		: k(_k)
		, trainingset(_trainingset)
		, d(_d)
	{}

	template<typename ROW>
	std::map<dependency_id_t, float> predict(ROW const& ys) const
	{
		best_set_t set(k);

		trainingset.citerate([&](typename MATRIX::const_row_proxy_t const& xs) {
			float d = distance::euclidean<decltype(xs.begin()->second), decltype(xs), ROW>(xs, ys);
			set.try_add(std::make_pair(xs.row_i, d));
		});

		std::map<dependency_id_t, float> suggestions;
		for(auto const& kvp : set.items)
		{
			float weight = 1.0f / (kvp.second + 1.0f); // "Similarity", higher is more similar

			for(auto dep_kvp : d.dependency_matrix[kvp.first])
				suggestions[dep_kvp.first] += ((float)dep_kvp.second) * weight;
		}

		return suggestions;
	}
};

template<typename MATRIX>
void knn<MATRIX>::best_set_t::try_add(knn::distance_t&& s)
{
	if(items.size() == k && s.second > items.back().second)
		return;

	auto it = std::upper_bound(items.begin(), items.end(), s, comp);

	if(items.size() < k)
	{
		items.emplace(it, std::move(s));
		return;
	}

	items.emplace(it, s);
	items.pop_back();
}

}
