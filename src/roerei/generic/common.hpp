#pragma once

#include <functional>
#include <iterator>
#include <vector>

namespace roerei
{

template<typename F, typename FARG>
using is_function = typename std::is_constructible<std::function<FARG>, F>;

template<typename XS, typename YS, typename F>
void set_intersect(XS const& xs, YS const& ys, F const& f)
{
	auto x_it = xs.begin();
	auto y_it = ys.begin();
	auto const x_it_end = xs.end();
	auto const y_it_end = ys.end();

	while(x_it != x_it_end && y_it != y_it_end)
	{
		if(*x_it < *y_it)
			x_it++;
		else if(*y_it < *x_it)
			y_it++;
		else
		{
			f(*x_it);
			x_it++;
			y_it++;
		}
	}
}

// Use only if ys is significantly smaller then xs
template<typename T, typename YS, typename F>
void set_binary_intersect(std::vector<T> const& xs, YS const& ys, F const& f)
{
	auto x_it = xs.begin();
	auto y_it = ys.begin();
	auto const x_it_end = xs.end();
	auto const y_it_end = ys.end();

	while(y_it != y_it_end)
	{
		x_it = std::lower_bound(x_it, x_it_end, *y_it); // Performs binary search

		if(x_it == x_it_end)
			break;

		if(*x_it == *y_it)
			f(*x_it);

		do
		{
			y_it++;
		}
		while(y_it != y_it_end && *y_it < *x_it);
	}
}

template<typename T, typename F>
void set_smart_intersect(std::vector<T> const& xs, std::vector<T> const& ys, F const& f)
{
	if(xs.size()/2 > ys.size())
		set_binary_intersect(xs, ys, f);
	else if(ys.size()/2 > xs.size())
		set_binary_intersect(ys, xs, f);
	else
		set_intersect(xs, ys, f);
}

namespace impl
{
	template<typename XS>
	size_t get_nonempty_size(XS const& xs, std::random_access_iterator_tag)
	{
		return std::distance(xs.begin(), xs.end());
	}

	template<typename XS, typename T>
	size_t get_nonempty_size(XS const& xs, T)
	{
		return xs.nonempty_size();
	}
}

template<typename XS>
size_t get_nonempty_size(XS const& xs)
{
	return impl::get_nonempty_size(xs, typename std::iterator_traits<decltype(xs.begin())>::iterator_category());
}

}
