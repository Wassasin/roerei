#pragma once

#include <vector>
#include <string>

#include <boost/optional.hpp>

#include <roerei/ml/ml_type.hpp>
#include <roerei/ml/posetcons_type.hpp>

namespace roerei
{

struct cli_options
{
	std::string action;
	std::vector<std::string> args;
	std::vector<std::string> corpii;
	std::vector<ml_type> methods;
	std::vector<posetcons_type> strats;
	boost::optional<std::string> filter;
	bool silent = false;
	bool prior = true;
	size_t jobs = 1;
};

}
