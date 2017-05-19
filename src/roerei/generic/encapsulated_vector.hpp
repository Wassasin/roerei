#pragma once

#include <vector>
#include <map>

namespace roerei
{

template<typename ID, typename T>
class encapsulated_vector
{
private:
	std::vector<T> buf;

public:
	encapsulated_vector() = default;
	encapsulated_vector(encapsulated_vector const&) = default;
	encapsulated_vector(encapsulated_vector&&) = default;
	encapsulated_vector(size_t n)
		: buf(n)
	{}
	encapsulated_vector(size_t n, T default_value)
		: buf(n, default_value)
	{}

	template<class... Args>
	void emplace_back(Args&&... args)
	{
		buf.emplace_back(args...);
	}

	void reserve(size_t n)
	{
		buf.reserve(n);
	}

	size_t size() const
	{
		return buf.size();
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
