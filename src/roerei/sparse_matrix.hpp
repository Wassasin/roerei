#pragma once

#include <roerei/common.hpp>

#include <boost/optional.hpp>

#include <vector>
#include <map>

namespace roerei
{

template <typename T>
class sparse_matrix_t
{
public:
	typedef std::map<size_t, T> row_t;

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
		row_proxy_base_t(MATRIX& _parent, size_t const _row_i)
			: parent(_parent)
			, row_i(_row_i)
		{}

	public:
		T& operator[](size_t j)
		{
			assert(j < parent.n);
			return parent.data[row_i][j];
		}

		T const& operator[](size_t j) const
		{
			assert(j < parent.n);
			try
			{
				return parent.data[row_i].at(j);
			}
			catch(std::out_of_range)
			{
				return 0;
			}
		}

		iterator begin() const
		{
			return parent.data[row_i].begin();
		}

		iterator end() const
		{
			return parent.data[row_i].end();
		}

		size_t size() const
		{
			return parent.n;
		}

		size_t nonempty_size() const
		{
			return parent.data[row_i].size();
		}
	};

	typedef row_proxy_base_t<sparse_matrix_t<T>, typename row_t::iterator> row_proxy_t;
	typedef row_proxy_base_t<sparse_matrix_t<T> const, typename row_t::const_iterator> const_row_proxy_t;

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

namespace detail
{

template<typename T, typename S>
struct serialize_value;

template<typename T, typename S>
struct serialize_value<sparse_matrix_t<T>, S>
{
	static inline void exec(S& s, std::string const& name, sparse_matrix_t<T> const& m)
	{
		s.write_object(name, 3);
		s.write("m", m.m);
		s.write("n", m.n);
		s.write_array("data", m.m);

		for(std::size_t i = 0; i < m.m; ++i)
		{
			auto const row = m[i];
			s.write_array("row", row.nonempty_size());

			for(auto const kvp : row)
			{
				s.write_array("kvp", 2);
				s.write("j", kvp.first);
				s.write("v", kvp.second);
			}
		}
	}
};

template<typename T, typename D>
struct deserialize_value;

template<typename T, typename D>
struct deserialize_value<sparse_matrix_t<T>, D>
{
	static inline sparse_matrix_t<T> exec(D& s, const std::string& name)
	{
		if(3 != s.read_object(name))
			throw std::runtime_error("Inconsistency");

		std::size_t m, n;
		s.read("m", m);
		s.read("n", n);
		std::size_t m_real = s.read_array("data");

		if(m != m_real)
			throw std::runtime_error("Inconsistency");

		sparse_matrix_t<T> result(m, n);
		for(std::size_t i = 0; i < m; ++i)
		{
			auto row = result[i];
			std::size_t sparse_els = s.read_array("row"); // TODO reserve row_t

			for(std::size_t t = 0; t < sparse_els; ++t)
			{
				if(2 != s.read_array("kvp"))
					throw std::runtime_error("Inconsistency");

				std::size_t j, v;
				s.read("j", j);
				s.read("v", v);

				row[j] = v;
			}
		}

		return result;
	}
};

}

}
