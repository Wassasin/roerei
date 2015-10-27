#pragma once

#include <vector>
#include <set>
#include <algorithm>

namespace roerei
{

class sparse_unit_matrix_t
{
	const size_t m, n;
	std::vector<std::set<size_t>> data;

	void transitive_helper(size_t i, std::set<size_t>& visited)
	{
		if(!visited.emplace(i).second)
			return;

		std::set<size_t> const old_row = data[i]; // Copy
		for(size_t j : old_row)
		{
			transitive_helper(j, visited);
			data[i].insert(data[j].begin(), data[j].end());
		}
	}

public:
	sparse_unit_matrix_t(sparse_unit_matrix_t&&) = default;
	sparse_unit_matrix_t(sparse_unit_matrix_t&) = delete;

	sparse_unit_matrix_t(size_t const _m, size_t const _n)
		: m(_m)
		, n(_n)
		, data(m)
	{}

	std::set<size_t> const& operator[](size_t const i) const
	{
		return data[i];
	}

	bool const& operator[](std::pair<size_t, size_t> const& p) const
	{
		return (data[p.first].find(p.second) == data[p.first].end());
	}

	void set(std::pair<size_t, size_t> const& p, bool value = true)
	{
		if(value)
			data[p.first].emplace_hint(data[p.first].end(), p.second);
		else
			data[p.first].erase(p.second);
	}

	size_t size_m() const
	{
		return m;
	}

	size_t size_n() const
	{
		return n;
	}

	void transitive()
	{
		assert(m == n);

		std::set<size_t> visited;
		for(size_t i = 0; i < m; ++i)
			transitive_helper(i, visited);
	}
};

}
