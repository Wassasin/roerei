#pragma once

#include <roerei/generic/common.hpp>
#include <roerei/generic/encapsulated_vector.hpp>

#include <boost/optional.hpp>

#include <vector>
#include <map>

namespace roerei
{

template<typename M, typename N, typename T, typename TEST_F, typename GENERATE_F>
class jit_matrix_t
{
public:
	typedef M row_key_t;
	typedef N column_key_t;

	template<typename MATRIX>
	class col_iterator : public std::iterator<std::forward_iterator_tag, std::pair<N, T>, size_t, const std::pair<N, T>*, std::pair<N, T>&> {
		MATRIX const& parent;

		M const row_i;
		size_t col_i;

	public:
		col_iterator(MATRIX const& _parent, M _row_i, size_t _col_i)
			: parent(_parent)
			, row_i(_row_i)
			, col_i(_col_i)
		{
			while (col_i < parent.n && !parent.test_f(row_i, N(col_i))) {
				col_i++;
			}
		}

		std::pair<N, T> operator*() const
		{
			std::cout << row_i.unseal() << ' ' << col_i << std::endl;
			return std::make_pair<N, T>(col_i, parent.generate_f(row_i, N(col_i)));
		}

		col_iterator& operator++()
		{
			do {
				col_i++;
			} while (col_i < parent.n && !parent.test_f(row_i, N(col_i)));

			std::cout << "Stopped at " << col_i;

			return *this;
		}

		bool operator==(col_iterator const& rhs) const {
			return col_i == rhs.col_i &&
					row_i == rhs.row_i &&
					((&parent) == (&rhs.parent));
		}

		bool operator!=(col_iterator const& rhs) const {
			return !(this->operator ==(rhs));
		}
	};

	template<typename MATRIX>
	class row_proxy_base_t
	{
	public:
		typedef MATRIX parent_t;

	private:
		friend MATRIX;
		MATRIX const& parent;

	public:
		M const row_i;

	private:
		row_proxy_base_t(MATRIX& _parent, M const _row_i)
			: parent(_parent)
			, row_i(_row_i)
		{}

	public:
		T const operator[](N j) const
		{
			assert(j < parent.n);
			return parent.generate_f(row_i, j);
		}

		col_iterator<MATRIX> begin() const
		{
			return col_iterator<MATRIX>(parent, row_i, 0);
		}

		col_iterator<MATRIX> end() const
		{
			return col_iterator<MATRIX>(parent, row_i, parent.n);
		}

		size_t size() const
		{
			return parent.n;
		}

		size_t nonempty_size() const
		{
			return std::distance(begin(), end());
		}
	};

	typedef row_proxy_base_t<jit_matrix_t<M, N, T, TEST_F, GENERATE_F> const> const_row_proxy_t;

private:
	const size_t m, n;
	TEST_F test_f;
	GENERATE_F generate_f;

public:
	jit_matrix_t(jit_matrix_t&&) = default;
	jit_matrix_t(jit_matrix_t&) = delete;

	jit_matrix_t(size_t _m, size_t _n, TEST_F&& _test_f, GENERATE_F&& _generate_f)
		: m(_m)
		, n(_n)
		, test_f(std::move(_test_f))
		, generate_f(std::move(_generate_f))
	{}

	const_row_proxy_t const operator[](M i) const
	{
		assert(i < m);
		return const_row_proxy_t(*this, i);
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
	void citerate(F const& f) const
	{
		for(size_t i = 0; i < m; ++i)
			f(const_row_proxy_t(*this, M(i)));
	}

	template<typename F>
	void iterate(F const& f) const
	{
		citerate(f);
	}
};

}
