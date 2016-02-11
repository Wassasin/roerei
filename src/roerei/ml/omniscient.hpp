#pragma once

#include <roerei/dataset.hpp>

#include <list>
#include <algorithm>

namespace roerei
{

class omniscient
{
private:
	dataset_t const& d;

public:
	omniscient(dataset_t const& _d)
		: d(_d)
	{}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& ys) const
	{
		std::vector<std::pair<dependency_id_t, float>> result;

		for(std::pair<dependency_id_t, dataset_t::value_t> kvp : d.dependency_matrix[ys.row_i])
			result.emplace_back(kvp.first, 1.0f);

		return result;
	}
};

}
