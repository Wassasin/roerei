#pragma once

#include <roerei/serialization/deserialize_common.hpp>

#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/adapted.hpp>

#include <boost/optional.hpp>

#include <cstddef>
#include <vector>
#include <type_traits>

namespace roerei
{
struct deserialize_inconsistency : public std::runtime_error
{
	deserialize_inconsistency()
		: std::runtime_error("Cannot deserialize: the subject does not conform to the struct definition")
	{}
};

template<typename T, typename D>
inline T deserialize(D& s, const std::string name);

namespace detail
{
template<typename T, typename D, typename N, typename... XS>
struct deserialize_itr;

template<typename T, typename D>
struct deserialize_value;

template<typename T, typename N>
using name_t = boost::fusion::extension::struct_member_name<T, N::value>;

template<typename T, typename N>
using type_t = typename boost::fusion::result_of::value_at<T, N>::type;

template<typename N>
using next_t = typename boost::mpl::next<N>::type;

template<typename T>
using size_t = typename boost::fusion::result_of::size<T>::type;

/* Primitives */
#define deserialize_value_primitive(T)\
	template<typename D>\
	struct deserialize_value<T, D>\
	{\
		static inline T exec(D& s, const std::string& name)\
		{\
			T x;\
			s.read(name, x);\
			return x;\
		}\
	};

deserialize_value_primitive(uint64_t)
deserialize_value_primitive(std::string)
deserialize_value_primitive(float)
deserialize_value_primitive(bool)

/* Generic case */
template<typename T, typename D>
struct deserialize_value
{
	static inline T exec(D& s, const std::string& name)
	{
		if(s.read_object(name) != size_t<T>::value)
			throw deserialize_inconsistency();

		return deserialize_itr<T, D, boost::mpl::int_<0>>::exec(s);
	}
};

/* Specialisations */
template<typename ID, typename T, typename D>
struct deserialize_value<encapsulated_vector<ID, T>, D>
{
	static inline encapsulated_vector<ID, T> exec(D& s, const std::string& name)
	{
		const std::string element_name = name + "_e";
		const std::size_t n = s.read_array(name);

		encapsulated_vector<ID, T> xs;
		xs.reserve(n);

		for(std::size_t i = 0; i < n; ++i)
			xs.emplace_back(std::move(deserialize<T>(s, element_name)));

		return xs;
	}
};

template<typename T, typename D>
struct deserialize_value<std::vector<T>, D>
{
	static inline std::vector<T> exec(D& s, const std::string& name)
	{
		const std::string element_name = name + "_e";
		const std::size_t n = s.read_array(name);

		std::vector<T> xs;
		xs.reserve(n);

		for(std::size_t i = 0; i < n; ++i)
			xs.emplace_back(std::move(deserialize<T>(s, element_name)));

		return xs;
	}
};

template<typename T, typename D>
struct deserialize_value<boost::optional<T>, D>
{
	static inline boost::optional<T> exec(D& s, const std::string name)
	{
		try
		{
			s.read_null(name);
			return boost::none;
		} catch(type_error)
		{
			return deserialize_value<T, D>::exec(s, name);
		}
	}
};

/* Mechanics */
template<typename T, typename D, typename N, typename... XS>
struct deserialize_itr
{
	static inline T exec(D& s, XS&&... xs)
	{
		using current_t = type_t<T, N>;

		current_t x = deserialize_value<current_t, D>::exec(s, name_t<T, N>::call());
		return deserialize_itr<T, D, next_t<N>, XS..., current_t>::exec(s, std::forward<XS>(xs)..., std::move(x));
	}
};

template<typename T, typename D, typename... XS>
struct deserialize_itr<T, D, size_t<T>, XS...>
{
	static inline T exec(D&, XS&&... xs)
	{
		return T({std::move(xs)...});
	}
};
}

template<typename T, typename D>
inline T deserialize(D& s, const std::string name)
{
	return detail::deserialize_value<T, D>::exec(s, name);
}

}
