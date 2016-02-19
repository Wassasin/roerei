#pragma once

#include <string>
#include <map>
#include <ostream>

namespace roerei
{

enum class ml_type
{
	knn,
	knn_adaptive,
	naive_bayes,
	omniscient,
	ensemble
};

inline std::string to_string(ml_type t)
{
	switch(t)
	{
	case ml_type::knn:
		return "knn";
	case ml_type::knn_adaptive:
		return "knn_adaptive";
	case ml_type::naive_bayes:
		return "naive_bayes";
	case ml_type::omniscient:
		return "omniscient";
	case ml_type::ensemble:
		return "ensemble";
	}

	throw std::logic_error("Unknown ml_type");
}

inline std::ostream& operator<<(std::ostream& os, ml_type t)
{
	return os << to_string(t);
}

inline ml_type to_ml_type(std::string const& str)
{
	static const std::map<std::string, ml_type> tmap({
		{"knn", ml_type::knn},
		{"knn_adaptive", ml_type::knn_adaptive},
		{"naive_bayes", ml_type::naive_bayes},
		{"nb", ml_type::naive_bayes},
		{"omniscient", ml_type::omniscient},
		{"omni", ml_type::omniscient},
		{"ensemble", ml_type::ensemble}
	});

	return tmap.at(str);
}

}
