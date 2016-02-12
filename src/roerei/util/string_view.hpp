#pragma once

#include <string>
#include <ostream>

namespace roerei
{

class string_view
{
public:
	static const size_t npos = std::string::npos;

private:
	std::string const& buffer;
	size_t start;
	size_t size;

	string_view(std::string const& _buffer, size_t _i, size_t _n)
		: buffer(_buffer)
		, start(_i)
		, size(_n)
	{}

public:
	string_view(std::string const& _buffer)
		: buffer(_buffer)
		, start(0)
		, size(_buffer.size())
	{}

	string_view(string_view const& _rhs) = default;
	string_view(string_view&& _rhs) = default;

	size_t get_start() const
	{
		return start;
	}

	size_t get_size() const
	{
		return size;
	}

	void remove_prefix(size_t n)
	{
		start += n;
		size -= n;
	}

	void remove_suffix(size_t n)
	{
		size -= n;
	}

	char operator[](size_t i) const
	{
		return buffer[start+i];
	}

	string_view substr(size_t i, size_t n = npos) const
	{
		if(n == npos || i + n > start + size)
			n = size-i;

		return string_view(buffer, start+i, n);
	}

	std::string::const_iterator begin() const
	{
		return buffer.begin() + start;
	}

	std::string::const_iterator end() const
	{
		return buffer.begin() + start + size;
	}

	std::string to_string() const
	{
		return std::string(begin(), end());
	}

	size_t find(char ch, size_t pos = 0) const
	{
		size_t result = buffer.find(ch, start+pos);
		if(result == buffer.npos || result > start+size)
			return buffer.npos;

		return result - start;
	}
};

inline std::ostream& operator<<(std::ostream& os, string_view const& rhs)
{
	os << rhs.to_string();
	return os;
}

}
