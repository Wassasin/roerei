#pragma once

#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>
#include <roerei/sparse_unit_matrix.hpp>

#include <vector>
#include <algorithm>
#include <iostream>

namespace roerei
{

template<typename MATRIX>
class naive_bayes
{
	static const float pi = 10, sigma = -15, tau = 20;

	dataset_t const& d;
	std::vector<dependency_id_t> const& allowed_dependencies;
	dependencies::dependant_matrix_t const& dependants;
	MATRIX const& trainingset;

private:
	template<typename ROW>
	float rank(dependency_id_t phi_id, ROW const& test_row) const
	{
		std::set<object_id_t> const& proofs_with_phi_all = dependants[phi_id];
		std::set<object_id_t> proofs_with_phi;

		{
			auto it = proofs_with_phi_all.begin();
			trainingset.citerate([&](typename std::remove_reference<MATRIX>::type::const_row_proxy_t const& row) {
				it = std::lower_bound(it, proofs_with_phi_all.end(), row.row_i);
				if(it != proofs_with_phi_all.end() && *it == row.row_i)
					proofs_with_phi.emplace(row.row_i);
			});
		}

		size_t P = proofs_with_phi.size();
		float log_p = std::log((float)P);

		float result = log_p;

		// For each j in test_row (f_j)
		// p_j = number of proofs which use phi, which also use facts described with type f_j (amongst others).

		//std::map<feature_id_t, size_t> ps_j;
		//trainingset.


		std::map<feature_id_t, size_t> ps_j;
		for(object_id_t proof_with_phi : proofs_with_phi)
		{
			auto const& phi_row = trainingset[proof_with_phi];
			auto x_it = phi_row.begin();
			auto y_it = test_row.begin();
			auto x_it_end = phi_row.end();
			auto y_it_end = test_row.end();

			while(x_it != x_it_end && y_it != y_it_end)
			{
				if(x_it->first < y_it->first)
					x_it++;
				else if(y_it->first < x_it->first)
					y_it++;
				else // Assert x_it->second > 0
				{
					ps_j[y_it->first]++;
					x_it++;
					y_it++;
				}
			}
		}

		for(auto const& kvp_j : test_row)
		{
			float p_j = ps_j[kvp_j.first];
			if(p_j == 0)
				result += kvp_j.second * sigma;
			else
				result += kvp_j.second * (std::log(pi * (float)p_j) - log_p);
		}

		return result;
	}

public:
	naive_bayes(dataset_t const& _d, dependencies::dependant_matrix_t const& _dependants, std::vector<dependency_id_t> const& _allowed_dependencies, MATRIX const& _trainingset)
		: d(_d)
		, allowed_dependencies(_allowed_dependencies)
		, dependants(_dependants)
		, trainingset(_trainingset)
	{}

	template<typename ROW>
	std::map<dependency_id_t, float> predict(ROW const& test_row) const
	{
		std::map<dependency_id_t, float> ranks;
		for(dependency_id_t phi_id : allowed_dependencies)
			ranks.emplace(std::make_pair(phi_id, rank(phi_id, test_row)));

		return ranks;
	}
};

}
