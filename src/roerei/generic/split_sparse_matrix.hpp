#pragma once

namespace roerei
{

template<typename MATRIX>
class split_sparse_matrix_t
{
public:
	typedef typename MATRIX::row_key_t row_key_t;

private:
	MATRIX const& data;
	row_key_t const end;

public:
	typedef typename MATRIX::const_row_proxy_t const_row_proxy_t;

	split_sparse_matrix_t(MATRIX const& _data, row_key_t const& _end)
		: data(_data)
		, end(_end)
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
		if(i >= end)
			throw std::out_of_range("Element not listed");

		return data[i];
	}

	template<typename F>
	void citerate(F const& f) const
	{
		data.citerate([&](typename MATRIX::const_row_proxy_t const& row) {
			if(row.row_i >= end)
				return;

			f(row);
		});
	}
};

}
