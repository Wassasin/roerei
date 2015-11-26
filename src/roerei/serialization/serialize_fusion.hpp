#pragma once

#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/struct.hpp>

#include <boost/optional.hpp>

namespace roerei
{
namespace detail
{

template<typename T, typename S, typename N>
struct serialize_itr;

template<typename T, typename S>
struct serialize_value;

/* Primitives */
#define serialize_value_primitive(T)\
	template<typename S> \
	struct serialize_value<T, S> \
	{ \
		static inline void exec(S& s, std::string const& name, T const& x) \
		{ \
			s.write(name, x); \
		} \
	};

serialize_value_primitive(uint64_t)
serialize_value_primitive(std::string)
serialize_value_primitive(float)
serialize_value_primitive(bool)

/* Generic case */
template<typename T, typename S>
struct serialize_value
{
	static inline void exec(S& s, std::string const& name, T const& x)
	{
		s.write_object(name, boost::fusion::result_of::size<T>::value);
		detail::serialize_itr<T, S, boost::mpl::int_<0>>::exec(s, x);
	}
};

/* Specializations */
template<typename ID, typename T, typename S>
struct serialize_value<encapsulated_vector<ID, T>, S>
{
	static inline void exec(S& s, std::string const& name, encapsulated_vector<ID, T> const& xs)
	{
		const std::string element_name = name + "_e";
		s.write_array(name, xs.size());
		for(T const& x : xs)
			serialize_value<T, S>::exec(s, element_name, x);
	}
};

template<typename T, typename S>
struct serialize_value<std::vector<T>, S>
{
	static inline void exec(S& s, std::string const& name, std::vector<T> const& xs)
	{
		const std::string element_name = name + "_e";
		s.write_array(name, xs.size());
		for(T const& x : xs)
			serialize_value<T, S>::exec(s, element_name, x);
	}
};

template<typename T, typename U, typename S>
struct serialize_value<std::pair<T, U>, S>
{
	static inline void exec(S& s, std::string const& name, std::pair<T, U> const& x)
	{
		s.write_array(name, 2);
		serialize_value<T, S>::exec(s, name + "_first", x.first);
		serialize_value<U, S>::exec(s, name + "_second", x.second);
	}
};

template<typename T, typename S>
struct serialize_value<boost::optional<T>, S>
{
	static inline void exec(S& s, std::string const& name, boost::optional<T> const& x)
	{
		if(x)
			serialize_value<T, S>::exec(s, name, x.get());
		else
			s.write_null(name);
	}
};

template<typename T, typename N>
using name_t = boost::fusion::extension::struct_member_name<T, N::value>;

template<typename T, typename N>
using type_t = typename boost::fusion::result_of::value_at<T, N>::type;

template<typename N>
using next_t = typename boost::mpl::next<N>::type;

template<typename T>
using size_t = typename boost::fusion::result_of::size<T>::type;

template<typename T, typename S, typename N>
struct serialize_itr
{
	static inline void exec(S& s, T const& x)
	{
		using current_t = type_t<T, N>;
		serialize_value<current_t, S>::exec(s, name_t<T, N>::call(), boost::fusion::at<N>(x));
		serialize_itr<T, S, next_t<N>>::exec(s, x);
	}
};

template<typename T, typename S>
struct serialize_itr<T, S, size_t<T>>
{
	static inline void exec(S&, T const&) {}
};

}

template<typename T, typename S>
void inline serialize(S& s, std::string const& name, T const& x)
{
	detail::serialize_value<T, S>::exec(s, name, x);
}

}
