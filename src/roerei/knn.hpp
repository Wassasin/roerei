#pragma once

#include <roerei/dataset.hpp>

#include <list>
#include <algorithm>

namespace roerei
{

class knn
{
public:
	typedef std::pair<size_t, float> distance_t;

private:
	struct best_set_t
	{
		size_t k;
		std::list<distance_t> items;

		static inline bool comp(distance_t const& x, distance_t const& y)
		{
			return x.second > y.second; // Reverse sorting; closer is better
		}

		best_set_t(size_t _k)
			: k(_k)
			, items()
		{}

		void try_add(distance_t const& s);

	};

private:
	size_t k;
	dataset_t::matrix_t const& trainingset;


public:
	knn(size_t const _k, dataset_t::matrix_t const& _trainingset)
		: k(_k)
		, trainingset(_trainingset)
	{}

	template<typename ROW>
	std::list<distance_t> predict(ROW const& row);
};

void knn::best_set_t::try_add(knn::distance_t const& s)
{
	auto it = std::lower_bound(items.begin(), items.end(), s, comp);

	if(items.size() < k)
	{
		items.emplace(it, s);
		return;
	}

	if(it == items.begin())
		return;

	items.emplace(it, s);
	items.pop_front();
}

template<typename ROW>
std::list<knn::distance_t> knn::predict(ROW const& row)
{
	best_set_t set(k);

	for(size_t i = 0; i < trainingset.m; ++i)
	{
		float d = distance::euclidean<dataset_t::value_t, dataset_t::matrix_t::const_row_proxy_t, ROW>(trainingset[i], row);
		set.try_add(std::make_pair(i, d));
	}

	return set.items;
}

}
