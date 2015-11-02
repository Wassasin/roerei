#pragma once

#include <roerei/uri.hpp>

namespace roerei
{

struct mapping_t
{
	std::string file;
	uri_t src, dest;
};

}
