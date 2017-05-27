#pragma once

namespace roerei {

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

}
