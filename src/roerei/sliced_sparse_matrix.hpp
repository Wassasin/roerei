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
	template<typename PARENT>
	class iterator_base_t
	{
	public:
		typedef typename decltype(keys)::const_iterator it_t;
	private:
		struct internal_t
		{
			PARENT& parent;
			it_t it;

			internal_t(PARENT& _parent, it_t _it)
				: parent(_parent)
				, it(_it)
			{}

			bool is_end() const
			{
				return it == parent.keys.end();
			}
		};

		boost::optional<internal_t> x;

	public:
		iterator_base_t()
			: x()
		{}

		iterator_base_t(PARENT& parent, it_t&& _it)
			: x(internal_t(parent, _it))
		{}

		decltype(x->parent.data[*x->it]) operator*() const
		{
			assert(x);
			return x->parent.data[*x->it];
		}

		iterator_base_t& operator++()
		{
			++x->it;
			return *this;
		}

		bool operator==(iterator_base_t const& rhs) const
		{
			if(x && rhs.x)
				return (
					x->it == rhs.x->it &&
					&x->parent == &rhs.x->parent
				);

			if(x)
				return x->is_end();

			if(rhs.x)
				return rhs.x->is_end();

			return true;
		}

		bool operator!=(iterator_base_t const& rhs) const
		{
			return !this->operator==(rhs);
		}
	};

	typedef iterator_base_t<sliced_sparse_matrix_t<MATRIX>> iterator;
	typedef iterator_base_t<sliced_sparse_matrix_t<MATRIX const> const> const_iterator;

public:
	sliced_sparse_matrix_t(MATRIX& _data)
		: data(_data)
		, keys()
	{}

	void add_key(size_t const i)
	{
		keys.emplace_hint(keys.end(), i);
	}

	size_t size() const
	{
		return keys.size();
	}

	iterator begin()
	{
		return iterator(*this, keys.begin());
	}

	iterator end()
	{
		return iterator();
	}

	const_iterator begin() const
	{
		return const_iterator(*this, keys.begin());
	}

	const_iterator end() const
	{
		return const_iterator();
	}
};

}
