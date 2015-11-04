#pragma once

#include <chrono>

namespace roerei
{

class timer
{
public:
	typedef std::chrono::steady_clock clock;

private:
	clock::time_point start;

public:
	timer()
		: start(clock::now())
	{}

	clock::duration diff() const
	{
		return clock::now() - start;
	}

	std::chrono::microseconds diff_msec() const
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(diff());
	}
};

}
