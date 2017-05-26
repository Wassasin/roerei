#pragma once

namespace roerei {

template<typename A, typename B, typename CMP_F, typename F>
void set_compute_intersect(
		A const& a_begin, A const& a_end,
		B const& b_begin, B const& b_end,
		CMP_F const& ge_cmp_f,
		F const& f
) {
	auto a_it = a_begin;
	auto b_it = b_begin;

	while (a_it != a_end && b_it != b_end) {
		if (!ge_cmp_f(*a_it, *b_it)) { // thus a < b
			a_it++;
		} else if(!ge_cmp_f(*b_it, *a_it)) { // thus b < a
			b_it++;
		} else { // thus a == b
			f(*a_it, *b_it);
			a_it++;
			b_it++;
		}
	}
}

}
