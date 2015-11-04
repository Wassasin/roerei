#pragma once

#include <functional>

namespace roerei
{

template<typename F>
class guard
{
public:

private:
	F _f;

public:
	guard(guard&&) = default;

	guard(guard&) = delete;
	void operator=(guard&) = delete;

	guard(F const& f)
		: _f(f)
	{}

	~guard()
	{
		_f();
	}
};

template<typename F>
guard<F> make_guard(F&& f)
{
	return guard<F>(std::move(f));
}

}
