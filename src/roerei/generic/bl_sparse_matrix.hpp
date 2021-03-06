#pragma once

#include <vector>

namespace roerei
{

template<typename MATRIX>
class bl_sparse_matrix_t
{
public:
	typedef typename MATRIX::row_key_t row_key_t;

private:
	MATRIX const& data;
	std::vector<row_key_t> const& bl;

public:
	typedef typename MATRIX::const_row_proxy_t const_row_proxy_t;

	bl_sparse_matrix_t(MATRIX const& _data, std::vector<row_key_t> const& _bl)
		: data(_data)
		, bl(_bl)
	{}

	size_t size_m() const
	{
		return data.size_m();
	}

	size_t size_n() const
	{
		return data.size_n();
	}

	const_row_proxy_t operator[](row_key_t const i) const
	{
		if(std::binary_search(bl.begin(), bl.end(), i))
			throw std::out_of_range("Element blacklisted");

		return data[i];
	}

	template<typename F>
	void citerate(F const& f) const
	{
		auto it = bl.begin();
		auto const it_end = bl.end();
		data.citerate([&](typename MATRIX::const_row_proxy_t const& row) {
			it = std::lower_bound(it, it_end, row.row_i);
			if(it != it_end && *it == row.row_i)
				return;

			f(row);
		});
	}
};

}
