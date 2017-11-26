#pragma once

#include <vector>
#include <algorithm>

namespace roerei
{

template<typename M, typename N>
class full_unit_matrix_t
{
	const size_t m, n;
	std::vector<bool> data;

	void transitive_helper(M i, std::vector<bool>& visited)
	{
		auto it = visited.begin() + i.unseal();

		if (*it) {
			return;
		}

		(*it) = true;

		citerate(i, [&](M j) {
			transitive_helper(j, visited);
			citerate(j, [&](M k) {
				set(std::make_pair(i, k));
			});
		});
	}

	template<typename F>
	bool dfs(M needle, M x, std::set<M>& visited, F const& f) const
	{
		if(!visited.emplace(x).second) {
			return false;
		}

		citerate(x, [&](M y) {
			if (y == needle) {
				f(y);
				return true;
			}

			if (dfs(needle, y, visited, f)) {
				f(y);
				return true;
			}
		});

		return false;
	}

	template<typename F>
	void topological_sort_visit(M const mi, std::set<M>& marked, F const& f) const
	{
		if (marked.find(mi) != marked.end()) {
			return;
		}

		citerate(mi, [&](N ni) {
			topological_sort_visit(ni, marked, f);
		});

		marked.emplace(mi);
		f(mi);
	}

public:
	full_unit_matrix_t(full_unit_matrix_t&&) = default;
	full_unit_matrix_t(full_unit_matrix_t const&) = default;

	full_unit_matrix_t(size_t const _m, size_t const _n)
		: m(_m)
		, n(_n)
		, data(m*n)
	{}

	bool operator[](std::pair<M, N> const& p) const
	{
		return data[p.first.unseal()*n+p.second.unseal()];
	}

	void set(std::pair<M, N> const& p, bool value = true)
	{
		data[p.first.unseal()*n+p.second.unseal()] = value;
	}

	template<typename F>
	void citerate(M mi, F&& f) const
	{
		auto it = data.begin() + mi.unseal()*n;
		auto it_end = data.begin() + (mi.unseal()+1)*n;

		size_t ni = 0;
		for (; it != it_end; ++it) {
			if (*it) {
				f(N(ni));
			}
			ni++;
		}
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

		std::vector<bool> visited(m);
		for(size_t i = 0; i < m; ++i) {
			transitive_helper(i, visited);
		}
	}

	full_unit_matrix_t<N, M> transpose() const
	{
		full_unit_matrix_t<N, M> result(n, m);
		for(size_t ni = 0; ni < n; ++ni) {
			for(size_t mi = 0; mi < m; ++mi) {
				if (this->operator [](std::make_pair(mi, ni))) {
					result.set(std::make_pair(ni, mi));
				}
			}
		}

		return result;
	}

	template<typename F>
	bool find_cyclic(F const& f) const
	{
		for(size_t i = 0; i < m; ++i) {
			std::set<M> visited;
			if (dfs(i, i, visited, f)) {
				f(i);
				return true;
			}
		}
		return false;
	}

	/**
	 * Implementation of Depth First Topological Sort
	 * Emits order of elements via f
	 */
	template<typename F>
	void topological_sort(F const& f) const
	{
		std::set<M> marked;
		while(marked.size() != size_m()) { // While there are unmarked nodes
			size_t i = 0;
			for(auto j : marked) {
				if (j.unseal() != i) {
					break;
				}
				++i;
			}
			topological_sort_visit(i, marked, f);
		}
	}
};

}
