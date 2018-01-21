#pragma once

#include <roerei/generic/encapsulated_vector.hpp>
#include <roerei/generic/full_unit_matrix.hpp>

#include <vector>
#include <set>
#include <algorithm>

namespace roerei
{

template<typename M, typename N>
class sparse_readonly_unit_matrix_t
{
	const size_t m, n;
	encapsulated_vector<M, std::vector<N>> data;

public:
	sparse_readonly_unit_matrix_t(sparse_readonly_unit_matrix_t&&) = default;
	//sparse_unit_matrix_t(sparse_unit_matrix_t&) = delete;
	sparse_readonly_unit_matrix_t(sparse_readonly_unit_matrix_t const&) = default;

	sparse_readonly_unit_matrix_t(full_unit_matrix_t<M, N> const& rhs)
		: m(rhs.size_m())
		, n(rhs.size_n())
		, data(rhs.size_m()) {
		for(size_t mi = 0; mi < m; ++mi) {
			auto& row = data[mi];
			rhs.citerate(mi, [&](N const ni) {
				row.emplace_back(ni);
			});
		}
	}

	std::vector<N> const& operator[](M const i) const
	{
		return data[i];
	}

	bool operator[](std::pair<M, N> const& p) const
	{
		auto it = std::lower_bound(data[p.first], p.second);

		return it != data[p.first].end() && *it == p.second;
	}

	size_t size_m() const
	{
		return m;
	}

	size_t size_n() const
	{
		return n;
	}
};

}
