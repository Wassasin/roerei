#pragma once

#include <string>
#include <map>
#include <ostream>

namespace roerei
{

enum class posetcons_type
{
	canonical,
	pessimistic,
	optimistic
};

std::string to_string(posetcons_type t)
{
	switch(t)
	{
	case posetcons_type::canonical:
		return "canonical";
	case posetcons_type::pessimistic:
		return "pessimistic";
	case posetcons_type::optimistic:
		return "optimistic";
	}

	throw std::logic_error("Unknown posetcons_type");
}

std::ostream& operator<<(std::ostream& os, posetcons_type t)
{
	return os << to_string(t);
}

posetcons_type to_posetcons_type(std::string const& str)
{
	static const std::map<std::string, posetcons_type> tmap({
		{"canonical", posetcons_type::canonical},
		{"pessimistic", posetcons_type::pessimistic},
		{"optimistic", posetcons_type::optimistic}
	});

	return tmap.at(str);
}

}
