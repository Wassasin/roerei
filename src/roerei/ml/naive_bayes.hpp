#pragma once

#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>

#include <roerei/generic/sparse_unit_matrix.hpp>
#include <roerei/generic/wl_sparse_matrix.hpp>

#include <roerei/util/performance.hpp>

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
	encapsulated_vector<feature_id_t, std::vector<object_id_t>> const& feature_occurance;
	std::vector<dependency_id_t> const& allowed_dependencies;
	dependencies::dependant_matrix_t const& dependants;
	MATRIX const& trainingset;

private:
	template<typename ROW>
	float rank(dependency_id_t phi_id, ROW const& test_row, std::vector<object_id_t> const& whitelist) const
	{
		// Feature -> set of objects with feature
		// Intersect with proofs which use phi
		// Fast value for p_j

		// For each j in test_row (f_j)
		// p_j = number of proofs which use phi, which also use facts described with type f_j (amongst others).

		// TODO encapsulate feature weight (might yield better performance)

		std::set<object_id_t> const& dependant_objs = dependants[phi_id];
		std::vector<object_id_t> candidates;
		candidates.reserve(std::min(whitelist.size(), dependant_objs.size())); // Upper bound reserve

		set_binary_intersect(whitelist, dependant_objs, [&](object_id_t i) {
			candidates.emplace_back(i);
		});

		if(candidates.empty())
			return -INFINITY;

		size_t P = candidates.size();
		float log_p = std::log((float)P);
		float result = log_p;

		for(auto const& kvp_j : test_row)
		{
			size_t p_j = 0;
			set_smart_intersect(feature_occurance[kvp_j.first], candidates, [&](object_id_t) { p_j++; });

			if(p_j == 0)
				result += kvp_j.second * sigma;
			else
				result += kvp_j.second * (std::log(pi * (float)p_j) - log_p);
		}

		return result;
	}

public:
	naive_bayes(dataset_t const& _d, decltype(feature_occurance) const& _feature_occurance, dependencies::dependant_matrix_t const& _dependants, std::vector<dependency_id_t> const& _allowed_dependencies, MATRIX const& _trainingset)
		: d(_d)
		, feature_occurance(_feature_occurance)
		, allowed_dependencies(_allowed_dependencies)
		, dependants(_dependants)
		, trainingset(_trainingset)
	{}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& test_row) const
	{
		std::vector<std::pair<dependency_id_t, float>> ranks;

		std::vector<object_id_t> feature_objs;
		feature_objs.reserve(trainingset.size_m());
		for(auto const& kvp_j : test_row)
		{
			auto const& xs = feature_occurance[kvp_j.first];
			feature_objs.insert(feature_objs.end(), xs.begin(), xs.end());
		}
		std::sort(feature_objs.begin(), feature_objs.end());
		std::unique(feature_objs.begin(), feature_objs.end());

		if(feature_objs.empty())
			return ranks;

		std::vector<object_id_t> trainingset_objs;
		trainingset_objs.reserve(trainingset.size_m());
		trainingset.citerate([&](typename std::remove_reference<MATRIX>::type::const_row_proxy_t const& row) {
			trainingset_objs.emplace_back(row.row_i);
		});

		std::vector<object_id_t> whitelist;
		whitelist.reserve(std::min(trainingset_objs.size(), feature_objs.size()));
		set_smart_intersect(trainingset_objs, feature_objs, [&](object_id_t i) {
			whitelist.emplace_back(i);
		});

		if(whitelist.empty())
			return ranks;

		ranks.reserve(allowed_dependencies.size());
		for(dependency_id_t phi_id : allowed_dependencies)
		{
			float r = rank(phi_id, test_row, whitelist);

			if(r == -INFINITY)
				continue;

			ranks.emplace_back(std::make_pair(phi_id, r));
		}

		return ranks;
	}
};

}
