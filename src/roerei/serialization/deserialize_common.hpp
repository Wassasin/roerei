#pragma once

#include <stdexcept>

namespace roerei
{

struct key_error : public std::runtime_error
{
	key_error(const std::string& expected, const std::string& received);
};

struct type_error : public std::runtime_error
{
	type_error(const std::string& expected, const std::string& received);
};

struct eob_error : public std::runtime_error
{
	eob_error();
};

key_error::key_error(const std::string& expected, const std::string& received)
	: std::runtime_error("Could not deserialize: unexpected key, expected " + expected + ", received " + received)
{}

type_error::type_error(const std::string& expected, const std::string& received)
	: std::runtime_error("Could not deserialize: unexpected type, expected " + expected + ", received " + received)
{}

eob_error::eob_error()
	: std::runtime_error("Could not deserialize: EOF, expected anything")
{}

}
