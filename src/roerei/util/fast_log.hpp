#pragma once

#include <cmath>
#include <cstdint>

namespace roerei
{

namespace impl
{

/* natural log on [0x1.f7a5ecp-127, 0x1.fffffep127]. Maximum relative error 9.4529e-5 */
float fast_log(float a)
{
	float m, r, s, t, i, f;
	int32_t e, m_int;

	e = (*reinterpret_cast<int32_t*>(&a) - 0x3f2aaaab) & 0xff800000;
	m_int = *reinterpret_cast<int32_t*>(&a) - e;
	m = *reinterpret_cast<float*>(&m_int);
	i = static_cast<float>(e) * 1.19209290e-7f; // 0x1.0p-23
	/* m in [2/3, 4/3] */
	f = m - 1.0f;
	s = f * f;
	/* Compute log1p(f) for f in [-1/3, 1/3] */
	r = fmaf (0.230836749f, f, -0.279208571f); // 0x1.d8c0f0p-3, -0x1.1de8dap-2
	t = fmaf (0.331826031f, f, -0.498910338f); // 0x1.53ca34p-2, -0x1.fee25ap-2
	r = fmaf (r, s, t);
	r = fmaf (r, s, f);
	r = fmaf (i, 0.693147182f, r); // 0x1.62e430p-1 // log(2)
	return r;
}

}

float fast_log(float a)
{
	return std::log(a);
}

}
