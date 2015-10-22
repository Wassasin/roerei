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
	MATRIX const& trainingset;

public:
	knn(size_t const _k, MATRIX const& _trainingset)
		: k(_k)
		, trainingset(_trainingset)
	{}

	template<typename ROW>
	std::list<distance_t> predict(ROW const& ys) const
	{
		best_set_t set(k);

		for(auto const& xs : trainingset)
		{
			float d = distance::euclidean<decltype(xs.begin()->second), decltype(xs), ROW>(xs, ys);
			set.try_add(std::make_pair(xs.row_i, d));
		}

		return set.items;
	}
};

template<typename MATRIX>
void knn<MATRIX>::best_set_t::try_add(knn::distance_t const& s)
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

}
