#pragma once

#include <roerei/sparse_matrix.hpp>

#include <boost/optional.hpp>

#include <set>

namespace roerei
{

template<typename MATRIX>
class sliced_sparse_matrix_t
{
private:
	MATRIX& data;
	std::set<size_t> keys;

public:
	typedef typename MATRIX::row_proxy_t row_proxy_t;
	typedef typename MATRIX::const_row_proxy_t const_row_proxy_t;

	sliced_sparse_matrix_t(MATRIX& _data, bool fill = true)
		: data(_data)
		, keys()
	{
		if(fill)
			data.iterate([&](typename MATRIX::const_row_proxy_t const& row) {
				add_key(row.row_i);
			});
	}

	void add_key(size_t const i)
	{
		keys.emplace_hint(keys.end(), i);
	}

	void try_remove_key(size_t const i)
	{
		keys.erase(i);
	}

	size_t size() const
	{
		return data.m;
	}

	template<typename F>
	void iterate(F const& f) const
	{
		for(size_t i : keys)
			f(data[i]);
	}
};

}
