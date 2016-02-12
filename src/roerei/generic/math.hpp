#pragma once

#include <cmath>
#include <sstream>

namespace roerei
{

inline float round(float x, float decs)
{
	float mult = std::pow(10.0f, decs);
	return std::round(x*mult)/mult;
}

template<typename T>
inline float absdiff(T const x, T const y)
{
	if(x > y)
		return x - y;
	else
		return y - x;
}

template<typename T>
std::string fill(T x, size_t len, char const p = '0')
{
	std::stringstream sstr;
	sstr << x;

	if(round(x, 0) == x)
		sstr << ".0";

	size_t curr_length = sstr.tellp();

	if(curr_length < len)
		for(len -= sstr.tellp(); len > 0; --len)
			sstr << p;

	return sstr.str();
}

}
