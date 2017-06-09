#pragma once

#include <iterator>

namespace roerei {

template<typename A, typename B, typename A_F, typename B_F>
bool set_compute_does_intersect(
		A const& a_begin, A const& a_end,
		B const& b_begin, B const& b_end,
		A_F const& a_f,
		B_F const& b_f
) {
	auto a_it = a_begin;
	auto b_it = b_begin;

	while (a_it != a_end && b_it != b_end) {
		auto ai = a_f(*a_it);
		auto bi = b_f(*b_it);
		if (ai < bi) {
			a_it++;
		} else if(bi < ai) {
			b_it++;
		} else { // thus a == b
			return true;
		}
	}

	return false;
}

template<typename A, typename B, typename A_F, typename B_F, typename F>
void set_compute_intersect(
		A const& a_begin, A const& a_end,
		B const& b_begin, B const& b_end,
		A_F const& a_f,
		B_F const& b_f,
		F const& f
) {
	auto a_it = a_begin;
	auto b_it = b_begin;

	while (a_it != a_end && b_it != b_end) {
		auto ai = a_f(*a_it);
		auto bi = b_f(*b_it);
		if (ai < bi) {
			a_it++;
		} else if(bi < ai) {
			b_it++;
		} else { // thus a == b
			f(*a_it, *b_it);
			a_it++;
			b_it++;
		}
	}
}

// Use only if b is significantly smaller then a
template<typename A, typename B, typename A_F, typename B_F, typename F>
void set_compute_binary_intersect(
		A const& a_begin, A const& a_end,
		B const& b_begin, B const& b_end,
		A_F const& a_f,
		B_F const& b_f,
		F const& f
) {
	auto a_it = a_begin;
	auto b_it = b_begin;

	auto bi = b_f(*b_it);

	while(b_it != b_end)
	{
		a_it = std::lower_bound(a_it, a_end, *b_it, [&](auto&& ap, auto&& bp) {
			return a_f(ap) < b_f(bp);
		}); // Performs binary search
		auto ai = a_f(*a_it);

		if(b_it == b_end)
			break;

		if(ai == bi)
			f(*a_it, *b_it);

		do
		{
			b_it++;
			bi = b_f(*b_it);
		}
		while(b_it != b_end && bi < ai);
	}
}

namespace impl
{
	template<typename A, typename B, typename A_F, typename B_F, typename F>
	void set_compute_smart_intersect(
			A const& a_begin, A const& a_end, size_t a_size,
			B const& b_begin, B const& b_end, size_t b_size,
			A_F const& a_f,
			B_F const& b_f,
			F const& f,
			std::random_access_iterator_tag,
			std::random_access_iterator_tag
	) {
		if(a_size/2 > b_size)
			set_compute_binary_intersect(a_begin, a_end, b_begin, b_end, a_f, b_f, f);
		else if(b_size/2 > a_size)
			set_compute_binary_intersect(b_begin, b_end, a_begin, a_end, b_f, a_f, [&f](auto&& b, auto&& a) { f(a, b); });
		else
			set_compute_intersect(a_begin, a_end, b_begin, b_end, a_f, b_f, f);
	}

	template<typename A, typename B, typename A_F, typename B_F, typename F, typename T>
	void set_compute_smart_intersect(
			A const& a_begin, A const& a_end, size_t a_size,
			B const& b_begin, B const& b_end, size_t b_size,
			A_F const& a_f,
			B_F const& b_f,
			F const& f,
			std::random_access_iterator_tag,
			T
	) {
		if(a_size/2 > b_size)
			set_compute_binary_intersect(a_begin, a_end, b_begin, b_end, a_f, b_f, f);
		else
			set_compute_intersect(a_begin, a_end, b_begin, b_end, a_f, b_f, f);
	}
}

template<typename A, typename B, typename A_F, typename B_F, typename F>
void set_compute_smart_intersect(
		A const& a_begin, A const& a_end, size_t a_size,
		B const& b_begin, B const& b_end, size_t b_size,
		A_F const& a_f,
		B_F const& b_f,
		F const& f)
{
	impl::set_compute_smart_intersect(
		a_begin, a_end, a_size, b_begin, b_end, b_size, a_f, b_f, f,
		typename std::iterator_traits<A>::iterator_category(),
		typename std::iterator_traits<B>::iterator_category()
	);
}

}
