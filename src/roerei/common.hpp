#pragma once

namespace std
{

template<bool B, class T = void>
using enable_if_t = typename enable_if<B,T>::type;

}

namespace roerei
{

template<typename F, typename FARG>
using is_function = typename std::is_constructible<std::function<FARG>, F>;

template<typename A, typename CONTAINER>
std::map<A, size_t> create_map(CONTAINER const& xs)
{
	std::map<A, size_t> result;

	size_t i = 0;
	for(auto const& x : xs)
		result.emplace(std::make_pair(x, i++));

	return result;
}

}
