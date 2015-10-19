#pragma once

#include <roerei/common.hpp>

#include <vector>
#include <map>

namespace roerei
{

template <typename T>
class sparse_matrix_t
{
public:
	typedef std::map<size_t, T> row_t;

	class row_proxy_t
	{
		friend class sparse_matrix_t<T>;

		sparse_matrix_t<T>& parent;
		row_t& row;

		row_proxy_t(sparse_matrix_t<T>& _parent, row_t& _row)
			: parent(_parent)
			, row(_row)
		{}

	public:
		T& operator[](size_t j)
		{
			assert(j < parent.n);
			return row[j];
		}

		typename row_t::iterator begin()
		{
			return row.begin();
		}

		typename row_t::iterator end()
		{
			return row.end();
		}
	};

	class const_row_proxy_t
	{
		friend class sparse_matrix_t<T>;

		sparse_matrix_t<T> const& parent;
		row_t const& row;

		const_row_proxy_t(sparse_matrix_t<T> const& _parent, row_t const& _row)
			: parent(_parent)
			, row(_row)
		{}

	public:
		T const& operator[](size_t j) const
		{
			assert(j < parent.n);
			return row[j];
		}

		typename row_t::const_iterator begin() const
		{
			return row.begin();
		}

		typename row_t::const_iterator end() const
		{
			return row.end();
		}
	};

public:
	const size_t m, n;

private:
	std::vector<row_t> data;

public:
	sparse_matrix_t(sparse_matrix_t&&) = default;
	sparse_matrix_t(sparse_matrix_t&) = delete;

	sparse_matrix_t(size_t _m, size_t _n)
		: m(_m)
		, n(_n)
		, data(m)
	{}

	row_proxy_t operator[](size_t i)
	{
		assert(i < m);
		return row_proxy_t(*this, data[i]);
	}

	const_row_proxy_t const operator[](size_t i) const
	{
		assert(i < m);
		return const_row_proxy_t(*this, data[i]);
	}
};

}
