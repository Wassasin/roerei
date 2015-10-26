#pragma once

#include <roerei/sparse_matrix.hpp>
#include <roerei/common.hpp>

#include <boost/optional.hpp>

#include <vector>
#include <map>

namespace roerei
{

template <typename T>
class compact_sparse_matrix_t
{
public:
	struct row_t
	{
		size_t const start, length;

		row_t(size_t _start, size_t _length)
			: start(_start)
			, length(_length)
		{}
	};

public:
	const size_t m, n;

private:
	std::vector<std::pair<size_t, T>> buf;
	std::vector<row_t> rows;

public:
	template<typename MATRIX, typename ITERATOR>
	class row_proxy_base_t
	{
	public:
		typedef ITERATOR iterator;
		typedef MATRIX parent_t;

	private:
		friend MATRIX;

		MATRIX& parent;

	public:
		size_t const row_i;

	private:
		iterator begin_it, end_it;

	private:
		row_proxy_base_t(MATRIX& _parent, size_t const _row_i)
			: parent(_parent)
			, row_i(_row_i)
			, begin_it(parent.buf.begin() + parent.rows[row_i].start)
			, end_it(begin_it + parent.rows[row_i].length)
		{}

	public:
		T& operator[](size_t j)
		{
			assert(j < parent.n);
			auto& it(std::lower_bound(begin(), end(), std::make_pair(j, 0), [](std::pair<size_t, T> const& x, std::pair<size_t, T> const& y) {
				return x.first < y.first;
			}));

			if(it == end() || it->first > j)
				throw std::out_of_range("Element is not present; cannot add as compact_sparse_matrix_t<T>");

			return it->second;
		}

		T const& operator[](size_t j) const
		{
			assert(j < parent.n);
			auto const& it(std::lower_bound(begin(), end(), std::make_pair(j, 0), [](std::pair<size_t, T> const& x, std::pair<size_t, T> const& y) {
				return x.first < y.first;
			}));

			if(it == end() || it->first > j)
				throw std::out_of_range("Element is not present; cannot add as compact_sparse_matrix_t<T>");

			return it->second;
		}

		iterator begin() const
		{
			return begin_it;
		}

		iterator end() const
		{
			return end_it;
		}

		size_t size() const
		{
			return parent.n;
		}

		size_t nonempty_size() const
		{
			return parent.rows[row_i].length;
		}
	};

	typedef row_proxy_base_t<compact_sparse_matrix_t<T>, decltype(buf.begin())> row_proxy_t;
	typedef row_proxy_base_t<compact_sparse_matrix_t<T> const, decltype(buf.cbegin())> const_row_proxy_t;

public:
	compact_sparse_matrix_t(compact_sparse_matrix_t&&) = default;
	compact_sparse_matrix_t(compact_sparse_matrix_t&) = delete;

	compact_sparse_matrix_t(sparse_matrix_t<T> const& mat)
		: m(mat.m)
		, n(mat.n)
		, buf()
		, rows()
	{
		rows.reserve(mat.m);

		size_t nonempty_elements = 0;
		size_t i = 0;
		mat.iterate([&](typename sparse_matrix_t<T>::const_row_proxy_t const& xs) {
			for(; i < xs.row_i; ++i)
				rows.emplace_back(nonempty_elements, 0);

			size_t size = xs.nonempty_size();
			rows.emplace_back(nonempty_elements, size);
			nonempty_elements += size;
			i++;
		});

		buf.reserve(nonempty_elements);
		mat.iterate([&](typename sparse_matrix_t<T>::const_row_proxy_t const& xs) {
			buf.insert(buf.end(), xs.begin(), xs.end());
		});
	}

	row_proxy_t operator[](size_t i)
	{
		assert(i < m);
		return row_proxy_t(*this, i);
	}

	const_row_proxy_t const operator[](size_t i) const
	{
		assert(i < m);
		return const_row_proxy_t(*this, i);
	}

	template<typename F>
	void iterate(F const& f)
	{
		for(size_t i = 0; i < m; ++i)
			f(row_proxy_t(*this, i));
	}

	template<typename F>
	void iterate(F const& f) const
	{
		for(size_t i = 0; i < m; ++i)
			f(const_row_proxy_t(*this, i));
	}
};

}
