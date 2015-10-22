#pragma once

#include <roerei/sparse_matrix.hpp>
#include <roerei/common.hpp>

#include <cmath>

namespace roerei
{

class distance
{
private:
	distance() = delete;

	template<typename T, typename VEC1, typename VEC2, typename F, typename = std::enable_if_t<is_function<F, void(size_t const, T const&, T const&)>::value>>
	static inline void yield_nonempty_pairs(VEC1 const& xs, VEC2 const& ys, F const& f)
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

		while(xs_it != xs.end())
		{
			f(xs_it->first, xs_it->second, 0);
			xs_it++;
		}

		while(ys_it != ys.end())
		{
			f(ys_it->first, 0, ys_it->second);
			ys_it++;
		}
	}

	template<typename T>
	static inline float absdiff(T const x, T const y)
	{
		if(x > y)
			return std::fdim(x, y);
		else
			return std::fdim(y, x);
	}

public:
	template<size_t P, typename T, typename VEC1, typename VEC2>
	static inline float minkowski(VEC1 const& xs, VEC2 const& ys)
	{
		static const float p = P;
		assert(xs.size() == ys.size());

		float sum = 0.0f;
		yield_nonempty_pairs<T, VEC1, VEC2>(xs, ys, [&](size_t const, T const& x, T const& y) {
			sum += std::pow(absdiff(x, y), p);
		});
		return std::pow(sum, 1.0f / P);
	}

	template<typename T, typename VEC1, typename VEC2>
	static inline float euclidean(VEC1 const& xs, VEC1 const& ys)
	{
		return minkowski<2, T, VEC1, VEC2>(xs, ys);
	}
};

}
