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

struct nb_preload_data_t
{
	nb_preload_data_t(nb_preload_data_t const&) = delete;
	nb_preload_data_t(nb_preload_data_t&&) = default;

	dependencies::dependant_matrix_t dependants;
	encapsulated_vector<object_id_t, std::vector<dependency_id_t>> allowed_dependencies;
	encapsulated_vector<feature_id_t, std::vector<object_id_t>> feature_occurance;

	nb_preload_data_t(dataset_t const& d)
		: dependants(dependencies::create_dependants(d))
		, allowed_dependencies(d.objects.size())
		, feature_occurance(d.features.size())
	{
		std::map<object_id_t, dependency_id_t> dependency_revmap(d.create_dependency_revmap());
		dependencies::dependant_obj_matrix_t dependants_trans(dependencies::create_obj_dependants(d));
		dependants_trans.transitive();

		d.objects.keys([&](object_id_t i) {
			std::vector<dependency_id_t> wl, bl;
			for(object_id_t forbidden : dependants_trans[i])
			{
				auto it = dependency_revmap.find(forbidden);
				if(it == dependency_revmap.end())
					continue;
				bl.emplace_back(it->second);
			}

			std::sort(bl.begin(), bl.end());

			auto it = bl.begin();
			d.dependencies.keys([&](dependency_id_t j) {
				it = std::lower_bound(it, bl.end(), j);
				if(it != bl.end() && *it == j)
					return;

				wl.emplace_back(j);
			});

			allowed_dependencies.emplace_back(std::move(wl));
		});

		d.feature_matrix.citerate([&](dataset_t::feature_matrix_t::const_row_proxy_t const& row) {
			for(auto const& kvp : row)
			{
				assert(kvp.second > 0);
				feature_occurance[kvp.first].emplace_back(row.row_i);
			}
		});
	}
};

template<typename MATRIX>
class naive_bayes
{
private:
	float const pi, sigma, tau;

	dataset_t const& d;
	nb_preload_data_t const& pld;
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

		std::set<object_id_t> const& dependant_objs = pld.dependants[phi_id];
		std::vector<object_id_t> candidates;
		candidates.reserve(std::min(whitelist.size(), dependant_objs.size())); // Upper bound reserve

		set_binary_intersect(whitelist, dependant_objs, [&](object_id_t i) {
			candidates.emplace_back(i);
		});

		if(candidates.empty())
			return -INFINITY;

		size_t P = candidates.size() + tau;
		float log_p = std::log(static_cast<float>(P));
		float result = log_p;

		for(auto const& kvp_j : test_row)
		{
			size_t p_j = tau;
			set_smart_intersect(pld.feature_occurance[kvp_j.first], candidates, [&](object_id_t) { p_j++; });

			if(p_j == 0)
				result += kvp_j.second * sigma;
			else
				result += kvp_j.second * (std::log(pi * static_cast<float>(p_j)) - log_p);
		}

		return result;
	}

public:
	naive_bayes(
			float _pi, // default 10
			float _sigma, // default -15
			float _tau, // default 20
			dataset_t const& _d,
			nb_preload_data_t const& _pld,
			MATRIX const& _trainingset
		)
		: pi(_pi)
		, sigma(_sigma)
		, tau(_tau)
		, d(_d)
		, pld(_pld)
		, trainingset(_trainingset)
	{}

	template<typename ROW>
	std::vector<std::pair<dependency_id_t, float>> predict(ROW const& test_row, object_id_t test_row_id) const
	{
		std::vector<std::pair<dependency_id_t, float>> ranks;

		std::vector<object_id_t> feature_objs;
		feature_objs.reserve(trainingset.size_m());
		for(auto const& kvp_j : test_row)
		{
			auto const& xs = pld.feature_occurance[kvp_j.first];
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

		ranks.reserve(pld.allowed_dependencies.size());
		for(dependency_id_t phi_id : pld.allowed_dependencies[test_row_id])
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
