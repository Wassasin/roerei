#pragma once

#include <vector>

namespace roerei
{

template<typename MATRIX, typename WL = std::vector<typename MATRIX::row_key_t>>
class wl_sparse_matrix_t
{
public:
	typedef typename MATRIX::row_key_t row_key_t;

private:
	MATRIX const& data;
	WL const& wl;

public:
	typedef typename MATRIX::const_row_proxy_t const_row_proxy_t;

	wl_sparse_matrix_t(MATRIX const& _data, WL const& _wl)
		: data(_data)
		, wl(_wl)
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
		if(!std::binary_search(wl.begin(), wl.end(), i))
			throw std::out_of_range("Element not listed");

		return data[i];
	}

	template<typename F>
	void citerate(F const& f) const
	{
		for(auto i : wl)
		{
			try
			{
				f(data[i]);
			} catch(std::out_of_range) {}
		}
	}
};

}
