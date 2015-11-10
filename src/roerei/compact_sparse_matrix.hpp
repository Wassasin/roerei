#pragma once

#include <roerei/sliced_sparse_matrix.hpp>
#include <roerei/sparse_matrix.hpp>
#include <roerei/encapsulated_vector.hpp>

#include <roerei/common.hpp>

#include <boost/optional.hpp>

#include <vector>
#include <map>

namespace roerei
{

template <typename M, typename N, typename T>
class compact_sparse_matrix_t
{
	static size_t constexpr invalid_value = std::numeric_limits<size_t>::max();

public:
	typedef M row_key_t;
	typedef N column_key_t;

	struct row_t
	{
		size_t const start, length;

		row_t(size_t _start, size_t _length)
			: start(_start)
			, length(_length)
		{}

		row_t()
			: start(invalid_value)
			, length(invalid_value)
		{}

		inline bool is_invalid() const
		{
			return start == invalid_value;
		}
	};

private:
	const size_t m, n;
	std::vector<std::pair<N, T>> buf;
	encapsulated_vector<M, row_t> rows;

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
		row_t const& row;

	public:
		M const row_i;

	private:
		row_proxy_base_t(MATRIX& _parent, row_t const& _row, M const _row_i)
			: parent(_parent)
			, row(_row)
			, row_i(_row_i)
		{
			assert(!row.is_invalid());
		}

	public:
		T& operator[](N j)
		{
			assert(j < parent.n);
			auto& it(std::lower_bound(begin(), end(), std::make_pair(j, invalid_value), [](std::pair<N, T> const& x, std::pair<N, T> const& y) {
				return x.first < y.first;
			}));

			if(it == end() || it->first > j)
				throw std::out_of_range("Element is not present; cannot add as compact_sparse_matrix_t<T>");

			return it->second;
		}

		T const& operator[](N j) const
		{
			assert(j < parent.n);
			auto const& it(std::lower_bound(begin(), end(), std::make_pair(j, invalid_value), [](std::pair<N, T> const& x, std::pair<N, T> const& y) {
				return x.first < y.first;
			}));

			if(it == end() || it->first > j)
				throw std::out_of_range("Element is not present");

			return it->second;
		}

		iterator begin() const
		{
			return parent.buf.data() + row.start;
		}

		iterator end() const
		{
			return begin() + row.length;
		}

		size_t size() const
		{
			return parent.n;
		}

		size_t nonempty_size() const
		{
			return row.length;
		}
	};

	typedef row_proxy_base_t<compact_sparse_matrix_t<M, N, T>, std::pair<N, T>*> row_proxy_t;
	typedef row_proxy_base_t<compact_sparse_matrix_t<M, N, T> const, std::pair<N, T> const*> const_row_proxy_t;

public:
	compact_sparse_matrix_t(compact_sparse_matrix_t&&) = default;
	compact_sparse_matrix_t(compact_sparse_matrix_t&) = delete;

	template<typename MATRIX>
	compact_sparse_matrix_t(MATRIX const& mat)
		: m(mat.size_m())
		, n(mat.size_n())
		, buf()
		, rows()
	{
		rows.reserve(mat.size_m());

		size_t nonempty_elements = 0;
		size_t i = 0;
		mat.citerate([&](typename MATRIX::const_row_proxy_t const& xs) {
			for(; i < xs.row_i.unseal(); ++i)
				rows.emplace_back();

			assert(i == xs.row_i);

			size_t size = xs.nonempty_size();
			rows.emplace_back(nonempty_elements, size);
			nonempty_elements += size;

			++i;
		});

		for(; i < m; ++i)
			rows.emplace_back();

		assert(rows.size() == m);

		buf.reserve(nonempty_elements);
		mat.citerate([&](typename MATRIX::const_row_proxy_t const& xs) {
			buf.insert(buf.end(), xs.begin(), xs.end());
		});
	}

	row_proxy_t operator[](M i)
	{
		assert(i < m);

		if(rows[i].is_invalid())
			throw std::out_of_range("Row is not present");

		return row_proxy_t(*this, rows[i], i);
	}

	const_row_proxy_t const operator[](M i) const
	{
		assert(i < m);

		if(rows[i].is_invalid())
			throw std::out_of_range("Row is not present");

		return const_row_proxy_t(*this, rows[i], i);
	}

	size_t size_m() const
	{
		return m;
	}

	size_t size_n() const
	{
		return n;
	}

	template<typename F>
	void iterate(F const& f)
	{
		for(size_t i = 0; i < m; ++i)
		{
			M im(i);
			if(!rows[i].is_invalid())
				f(row_proxy_t(*this, rows[i], M(i)));
		}
	}

	template<typename F>
	void citerate(F const& f) const
	{
		for(size_t i = 0; i < m; ++i)
		{
			M im(i);
			if(!rows[im].is_invalid())
				f(const_row_proxy_t(*this, rows[im], im));
		}
	}

	template<typename F>
	void iterate(F const& f) const
	{
		citerate(f);
	}
};

}
