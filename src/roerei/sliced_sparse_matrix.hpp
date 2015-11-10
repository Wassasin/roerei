#pragma once

#include <roerei/sparse_matrix.hpp>

#include <boost/optional.hpp>

#include <set>

namespace roerei
{

template<typename MATRIX>
class sliced_sparse_matrix_t
{
public:
	typedef typename MATRIX::row_key_t row_key_t;
	typedef typename MATRIX::column_key_t column_key_t;

private:
	MATRIX& data;
	std::set<row_key_t> keys;

public:
	typedef typename MATRIX::row_proxy_t row_proxy_t;
	typedef typename MATRIX::const_row_proxy_t const_row_proxy_t;

	sliced_sparse_matrix_t(MATRIX& _data, bool fill = true)
		: data(_data)
		, keys()
	{
		if(fill)
			data.citerate([&](typename MATRIX::const_row_proxy_t const& row) {
				add_key(row.row_i);
			});
	}

	void add_key(row_key_t const i)
	{
		keys.emplace_hint(keys.end(), i);
	}

	void try_remove_key(row_key_t const i)
	{
		keys.erase(i);
	}

	size_t nonempty_size_m() const
	{
		return keys.size();
	}

	size_t size_m() const
	{
		return data.size_m();
	}

	size_t size_n() const
	{
		return data.size_n();
	}

	template<typename F>
	void iterate(F const& f)
	{
		for(row_key_t i : keys)
			f(data[i]);
	}

	template<typename F>
	void citerate(F const& f) const
	{
		for(row_key_t i : keys)
		{
			auto const& d = data;
			f(d[i]);
		}
	}

	template<typename F>
	void iterate(F const& f) const
	{
		citerate(f);
	}
};

}
