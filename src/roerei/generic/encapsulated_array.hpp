#pragma once

#include <array>

namespace roerei
{

template<typename ID, typename T, size_t N>
class encapsulated_array
{
private:
	std::array<T, N> buf;

public:
	encapsulated_array() = default;
	encapsulated_array(encapsulated_array const&) = default;
	encapsulated_array(encapsulated_array&&) = default;
	encapsulated_array(std::initializer_list<T> xs)
		: buf(xs)
	{}


	encapsulated_array& operator=(encapsulated_array const&) = default;
	encapsulated_array& operator=(encapsulated_array&&) = default;

	size_t size() const
	{
		return N;
	}

	T& operator[](ID const& id)
	{
		return buf[id.unseal()];
	}

	T const& operator[](ID const& id) const
	{
		return buf[id.unseal()];
	}

	template<typename F> void keys(F const& f) const
	{
		ID::iterate(f, this->size());
	}

	template<typename F> void iterate(F const& f) const
	{
		ID::iterate([&](ID it) {
			f(it, buf[it.unseal()]);
		} , this->size());
	}

	decltype(buf.begin()) begin()
	{
		return buf.begin();
	}

	decltype(buf.end()) end()
	{
		return buf.end();
	}

	decltype(buf.cbegin()) begin() const
	{
		return buf.begin();
	}

	decltype(buf.cend()) end() const
	{
		return buf.end();
	}

	bool operator==(encapsulated_vector<ID, T> const& rhs) const
	{
		return buf == rhs.buf;
	}
};

}
