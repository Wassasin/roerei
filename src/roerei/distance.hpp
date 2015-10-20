#pragma once

#include <roerei/sparse_matrix.hpp>
#include <roerei/common.hpp>

#include <cmath>

namespace roerei
{

template<typename T>
class distance
{
public:
	typedef typename sparse_matrix_t<T>::const_row_proxy_t row_t;

private:
	distance() = delete;

	template<typename F, typename = std::enable_if_t<is_function<F, void(size_t const, T const&, T const&)>::value>>
	static inline void yield_nonempty_pairs(row_t const& xs, row_t const& ys, F const& f)
	{
		auto xs_it = xs.begin();
		auto ys_it = ys.begin();

		while(xs_it != xs.end() && ys_it != ys.end())
		{
			if(xs_it->first == ys_it->first)
			{
				f(xs_it->first, xs_it->second, ys_it->second);
				xs_it++;
				ys_it++;
			}
			else if(xs_it->first < ys_it->first)
			{
				f(xs_it->first, xs_it->second, 0);
				xs_it++;
			}
			else
			{
				f(ys_it->first, 0, ys_it->second);
				ys_it++;
			}
		}
	}

public:
	template<size_t P>
	static inline float minkowski(row_t const& xs, row_t const& ys)
	{
		static const float p = P;
		assert(xs.size() == ys.size());

		float sum = 0.0f;
		yield_nonempty_pairs(xs, ys, [&](size_t const i, T const& x, T const& y) {
			sum += std::pow(std::fdim(x, y), p);
		});
		return std::pow(sum, 1.0f / P);
	}

	static inline float euclidean(row_t const& xs, row_t const& ys)
	{
		return minkowski<2>(xs, ys);
	}
};

}
