#pragma once

#include <set>

namespace roerei
{

template<typename MATRIX>
class bl_sparse_matrix_t
{
	MATRIX const& data;
	std::vector<size_t> const bl;

public:
	typedef typename MATRIX::const_row_proxy_t const_row_proxy_t;

	template<typename CONTAINER>
	bl_sparse_matrix_t(MATRIX const& _data, CONTAINER const& _bl)
		: data(_data)
		, bl(_bl.begin(), _bl.end())
	{}

	template<typename F>
	void citerate(F const& f) const
	{
		auto it = bl.begin();
		data.citerate([&](typename MATRIX::const_row_proxy_t const& row) {
			it = std::lower_bound(it, bl.end(), row.row_i);
			if(it != bl.end() && *it == row.row_i)
				return;

			f(row);
		});
	}
};

}
