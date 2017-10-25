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

	void transitive_helper(M s, M v, sparse_unit_matrix_t<M, M>& tc) const
	{
		tc.set(std::make_pair(s, v));

		for(M i : data[v]) {
			if (!tc[std::make_pair(s, i)]) {
				transitive_helper(s, i, tc);
			}
		}
	}

	template<typename F>
	bool dfs(M needle, M x, std::set<M>& visited, F const& f) const
	{
		if(!visited.emplace(x).second) {
			return false;
		}

		for(M y : data[x]) {
			if (y == needle) {
				f(y);
				return true;
			}

			if (dfs(needle, y, visited, f)) {
				f(y);
				return true;
			}
		}

		return false;
	}

public:
	sparse_unit_matrix_t(sparse_unit_matrix_t&&) = default;
	//sparse_unit_matrix_t(sparse_unit_matrix_t&) = delete;
	sparse_unit_matrix_t(sparse_unit_matrix_t&) = default;

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
		return (data[p.first].find(p.second) != data[p.first].end());
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

	void transitive()
	{
		assert(m == n);

		sparse_unit_matrix_t<M, M> tc(m, m);
		for(size_t i = 0; i < m; ++i) {
			transitive_helper(i, i, tc);
		}

		for(size_t i = 0; i < m; ++i) {
			set(i, tc[i]);
		}
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
	 * Implementation of Kahn's algorithm
	 * Emits order of elements via f
	 */
	template<typename F>
	void topological_sort(F const& f) const
	{
		auto matrix = *this;

		auto find_nodes_without_incoming_f = [&]() {
			std::set<M> s;
			M::iterate([&s](M i) {
				s.emplace(i);
			});

			// Erase all nodes with incoming edges
			M::iterate([&s](M i) {
				s.erase(matrix.data[i].begin(), matrix.data[i].end());
			});
			return s;
		};

		std::set<M> s(find_nodes_without_incoming_f());
		while (!s.empty()) {
			auto it = s.begin();
			M n = *it;
			s.erase(it);

			f(n);
			std::set<M> e = data[n]; // Make copy
			matrix.data[n].clear();

			std::set<M> ss(find_nodes_without_incoming_f());
			std::set_intersection(
				e.begin(), e.end(),
				ss.begin(), ss.end(),
				std::inserter(s)
			);
		}

		// If not empty then we were not operating on a DAG
		M::iterate([&matrix](M i) {
			assert(matrix.data[i].empty());
		});
	}
};

}
