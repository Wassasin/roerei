#pragma once

#include <roerei/compact_sparse_matrix.hpp>
#include <roerei/sparse_matrix.hpp>
#include <roerei/math.hpp>

#include <cmath>

namespace roerei
{

namespace detail
{
	template<typename T, typename VEC1, typename VEC2, typename F>
	static inline void yield_nonempty_pairs(VEC1 const& xs, VEC2 const& ys, F&& f)
	{
		auto xs_it = xs.begin();
		auto ys_it = ys.begin();
		auto const xs_it_end = xs.end();
		auto const ys_it_end = ys.end();

		while(xs_it != xs_it_end && ys_it != ys_it_end)
		{
			if(xs_it->first == ys_it->first)
			{
				f(xs_it->second, ys_it->second);
				xs_it++;
				ys_it++;
			}
			else if(xs_it->first < ys_it->first)
			{
				f(xs_it->second, 0);
				xs_it++;
			}
			else
			{
				f(0, ys_it->second);
				ys_it++;
			}
		}

		while(xs_it != xs_it_end)
		{
			f(xs_it->second, 0);
			xs_it++;
		}

		while(ys_it != ys_it_end)
		{
			f(0, ys_it->second);
			ys_it++;
		}
	}
}

class distance
{
private:
	distance() = delete;

public:
	template<size_t P, typename T, typename VEC1, typename VEC2>
	static inline float minkowski(VEC1 const& xs, VEC2 const& ys)
	{
		static const float p = P;
		assert(xs.size() == ys.size());

		float sum = 0.0f;
		detail::yield_nonempty_pairs<T, VEC1 const&, VEC2 const&>(xs, ys, [&](T const x, T const y) {
			sum += std::pow(absdiff(x, y), p);
		});
		return std::pow(sum, 1.0f / P);
	}

	template<typename T, typename VEC1, typename VEC2>
	static inline float euclidean(VEC1 const& xs, VEC2 const& ys)
	{
		assert(xs.size() == ys.size());

		float sum = 0.0f;
		detail::yield_nonempty_pairs<T, VEC1 const&, VEC2 const&>(xs, ys, [&](T const x, T const y) {
			float v = (float)x - (float)y;
			sum += v * v;
		});
		return std::sqrt(sum);
	}
};

}
