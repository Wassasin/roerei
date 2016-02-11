#pragma once

#include <roerei/generic/common.hpp>
#include <roerei/generic/encapsulated_vector.hpp>

#include <boost/optional.hpp>

#include <vector>
#include <map>

namespace roerei
{

template<typename M, typename N, typename T>
class sparse_matrix_t
{
public:
	typedef M row_key_t;
	typedef N column_key_t;

	typedef std::map<N, T> row_t;

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
		M const row_i;

	private:
		row_proxy_base_t(MATRIX& _parent, M const _row_i)
			: parent(_parent)
			, row_i(_row_i)
		{}

	public:
		T& operator[](N j)
		{
			assert(j < parent.n);
			return parent.data[row_i][j];
		}

		T const operator[](N j) const
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

	typedef row_proxy_base_t<sparse_matrix_t<M, N, T>, typename row_t::iterator> row_proxy_t;
	typedef row_proxy_base_t<sparse_matrix_t<M, N, T> const, typename row_t::const_iterator> const_row_proxy_t;

private:
	const size_t m, n;
	encapsulated_vector<M, row_t> data;

public:
	sparse_matrix_t(sparse_matrix_t&&) = default;
	sparse_matrix_t(sparse_matrix_t&) = delete;

	sparse_matrix_t(size_t _m, size_t _n)
		: m(_m)
		, n(_n)
		, data(m)
	{}

	row_proxy_t operator[](M i)
	{
		assert(i < m);
		return row_proxy_t(*this, i);
	}

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
	void iterate(F const& f)
	{
		for(size_t i = 0; i < m; ++i)
			f(row_proxy_t(*this, M(i)));
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

namespace detail
{

template<typename T, typename S>
struct serialize_value;

template<typename M, typename N, typename T, typename S>
struct serialize_value<sparse_matrix_t<M, N, T>, S>
{
	static inline void exec(S& s, std::string const& name, sparse_matrix_t<M, N, T> const& m)
	{
		s.write_object(name, 3);
		s.write("m", m.size_m());
		s.write("n", m.size_n());
		s.write_array("data", m.size_m());

		m.citerate([&](typename sparse_matrix_t<M, N, T>::const_row_proxy_t const& row) {
			s.write_array("row", row.nonempty_size());

			for(auto const kvp : row)
			{
				s.write_array("kvp", 2);
				s.write("j", kvp.first.unseal());
				s.write("v", kvp.second);
			}
		});
	}
};

template<typename T, typename D>
struct deserialize_value;

template<typename M, typename N, typename T, typename D>
struct deserialize_value<sparse_matrix_t<M, N, T>, D>
{
	static inline sparse_matrix_t<M, N, T> exec(D& s, const std::string& name)
	{
		if(3 != s.read_object(name))
			throw std::runtime_error("Inconsistency");

		std::size_t m, n;
		s.read("m", m);
		s.read("n", n);
		std::size_t m_real = s.read_array("data");

		if(m != m_real)
			throw std::runtime_error("Inconsistency");

		sparse_matrix_t<M, N, T> result(m, n);
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
