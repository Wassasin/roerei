#pragma once

#include <roerei/generic/encapsulated_vector.hpp>

#include <vector>
#include <set>
#include <algorithm>

namespace roerei
{

template<typename M, typename N>
class sparse_unit_matrix_t
{
	const size_t m, n;
	encapsulated_vector<M, std::set<N>> data;

	void transitive_helper(M i, std::set<M>& visited)
	{
		if(!visited.emplace(i).second)
			return;

		std::set<M> const old_row = data[i]; // Copy
		for(M j : old_row)
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

	std::set<N> const& operator[](M const i) const
	{
		return data[i];
	}

	bool operator[](std::pair<M, N> const& p) const
	{
		return (data[p.first].find(p.second) == data[p.first].end());
	}

	void set(std::pair<M, N> const& p, bool value = true)
	{
		if(value)
			data[p.first].emplace_hint(data[p.first].end(), p.second);
		else
			data[p.first].erase(p.second);
	}

	void set(M const& i, std::set<N> const& js)
	{
		data[i].insert(js.begin(), js.end());
	}

	size_t size_m() const
	{
		return m;
	}

	size_t size_n() const
	{
		return n;
	}

	template<typename F>
	void citerate(F const& f) const
	{
		for(size_t i = 0; i < m; ++i)
		{
			f(std::make_pair(M(i), data[i]));
		}
	}

	void transitive()
	{
		assert(m == n);

		std::set<M> visited;
		for(size_t i = 0; i < m; ++i)
			transitive_helper(i, visited);
	}

	sparse_unit_matrix_t<N, M> transpose() const
	{
		sparse_unit_matrix_t<N, M> result(n, m);
		data.iterate([&result](M i, std::set<N> const& js) {
			for(N j : js)
				result.data[j].insert(i);
		});
		return result;
	}
};

}
