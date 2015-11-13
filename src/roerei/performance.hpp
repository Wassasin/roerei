#pragma once

#include <roerei/dataset.hpp>
#include <roerei/math.hpp>

#include <roerei/util/performance.hpp>

#include <algorithm>
#include <list>
#include <map>
#include <set>

#include <iostream>

namespace roerei
{

class performance
{
private:
	performance() = delete;

public:
	struct metrics_t
	{
		float oocover, ooprecision, recall, rank, auc;
		size_t n;

		metrics_t()
			: oocover(1.0f)
			, ooprecision(1.0f)
			, recall(std::numeric_limits<float>::max())
			, rank(std::numeric_limits<float>::max())
			, auc(0.0f)
			, n(0)
		{}

		metrics_t(float _oocover, float _ooprecision, float _recall, float _rank, float _auc)
			: oocover(_oocover)
			, ooprecision(_ooprecision)
			, recall(_recall)
			, rank(_rank)
			, auc(_auc)
			, n(1)
		{}

		metrics_t(float _oocover, float _ooprecision, float _recall, float _rank, float _auc, size_t _n)
			: oocover(_oocover)
			, ooprecision(_ooprecision)
			, recall(_recall)
			, rank(_rank)
			, auc(_auc)
			, n(_n)
		{}

		metrics_t operator+(metrics_t const& rhs) const
		{
			if(n == 0)
				return rhs;
			else if(rhs.n == 0)
				return *this;

			float total = n + rhs.n;

			float nr_f = (float)n / total;
			float rhs_nr_f = (float)rhs.n / total;

			return {
				oocover * nr_f + rhs.oocover * rhs_nr_f,
				ooprecision * nr_f + rhs.ooprecision * rhs_nr_f,
				recall * nr_f + rhs.recall * rhs_nr_f,
				rank * nr_f + rhs.rank * rhs_nr_f,
				auc * nr_f + rhs.auc * rhs_nr_f,
				total
			};
		}

		metrics_t& operator+=(metrics_t const& rhs)
		{
			*this = operator+(rhs);
			return *this;
		}
	};

	struct result_t
	{
		metrics_t metrics;
		std::vector<std::pair<dependency_id_t, float>> suggestions_sorted;
		std::vector<dependency_id_t> required_deps, oosuggested_deps, oofound_deps;
	};

	static std::pair<float, float> compute_recall_rank(float const c_found, float const c_suggested, std::vector<dependency_id_t> const& required_deps, std::vector<std::pair<dependency_id_t, float>> const& suggestions_sorted)
	{
		performance_scope("recall rank")

		if(required_deps.empty())
			return std::make_pair(0.0f, 0.0f);

		float recall = suggestions_sorted.size() + 1.0f;
		float rank = 0.0f;

		bool find_recall = c_found == required_deps.size();
		bool find_rank = c_found > 0;

		size_t j = 0;
		if(find_recall || find_rank)
		{
			std::set<dependency_id_t> todo_deps(required_deps.begin(), required_deps.end()); // Copy
			for(; j < suggestions_sorted.size() && !todo_deps.empty(); ++j)
				if(todo_deps.erase(suggestions_sorted.at(j).first) > 0)
					rank += j;
		}

		if(find_recall)
			recall = j;

		if(find_rank)
			rank /= c_found;
		else
			rank = c_suggested + 1.0f;

		return std::make_pair(recall, rank);
	}

	static float compute_auc(float c_required, std::vector<dependency_id_t> const& found_deps, std::vector<dependency_id_t> const& irrelevant_deps, std::map<dependency_id_t, size_t> const& suggestions_ranks)
	{
		// TODO this can be computed much faster with moving iterators

		performance_scope("auc")
		auto sr_f([&](dependency_id_t i) -> size_t { return suggestions_ranks.find(i)->second; }); // Cannot be std::map::end

		if(c_required == 0)
			return 1.0f;

		if(irrelevant_deps.empty())
			return 1.0f;

		if(found_deps.empty())
			return 0.0f;

		std::vector<size_t> found_ranks, irrelevant_ranks;
		found_ranks.reserve(found_deps.size());
		for(dependency_id_t i : found_deps)
			found_ranks.emplace_back(sr_f(i));

		irrelevant_ranks.reserve(irrelevant_deps.size());
		for(dependency_id_t j : irrelevant_deps)
			irrelevant_ranks.emplace_back(sr_f(j));

		float auc_sum = 0.0f;
		for(size_t x : found_ranks)
			for(size_t y : irrelevant_ranks)
				if(x < y) // Lower rank is 'better'
					auc_sum += 1.0f;

		return auc_sum / (float)(found_deps.size() * irrelevant_deps.size());
	}

	static result_t measure(dataset_t const& d, object_id_t test_row_i, std::vector<std::pair<dependency_id_t, float>> suggestions) noexcept
	{
		performance_scope("measure_analyze")

		std::sort(suggestions.begin(), suggestions.end(), [&](std::pair<dependency_id_t, float> const& x, std::pair<dependency_id_t, float> const& y) {
			return x.second > y.second;
		});

		std::map<dependency_id_t, size_t> suggestions_ranks;
		for(size_t j = 0; j < suggestions.size(); ++j)
			suggestions_ranks[suggestions[j].first] = j;

		std::vector<dependency_id_t> required_deps, suggested_deps, oosuggested_deps, found_deps, oofound_deps, missing_deps, irrelevant_deps;
		for(auto const& kvp : d.dependency_matrix[test_row_i])
			required_deps.emplace_back(kvp.first);

		for(size_t j = 0; j < suggestions.size(); ++j)
			suggested_deps.emplace_back(suggestions[j].first);

		if(suggested_deps.size() > 100)
		{
			oosuggested_deps.insert(oosuggested_deps.end(), suggested_deps.begin(), suggested_deps.begin()+100);
			std::sort(suggested_deps.begin(), suggested_deps.end()); // After copy
			std::sort(oosuggested_deps.begin(), oosuggested_deps.end());
		}
		else
		{
			std::sort(suggested_deps.begin(), suggested_deps.end()); // Before copy
			oosuggested_deps = suggested_deps;
		}

		std::set_intersection(
			required_deps.begin(), required_deps.end(),
			oosuggested_deps.begin(), oosuggested_deps.end(),
			std::inserter(oofound_deps, oofound_deps.begin())
		);

		std::set_intersection(
			required_deps.begin(), required_deps.end(),
			suggested_deps.begin(), suggested_deps.end(),
			std::inserter(found_deps, found_deps.begin())
		);

		std::set_difference(
			suggested_deps.begin(), suggested_deps.end(),
			found_deps.begin(), found_deps.end(),
			std::inserter(irrelevant_deps, irrelevant_deps.begin())
		);

		float c_required = required_deps.size();
		float c_found = found_deps.size();
		float c_suggested = suggested_deps.size();
		float c_oosuggested = oosuggested_deps.size();
		float c_oofound = oofound_deps.size();

		float oocover = c_oofound/c_required;
		float ooprecision = c_oofound/c_oosuggested;

		if(c_oofound == 0.0f)
			ooprecision = 0.0f;

		if(c_required == 0.0f)
		{
			oocover = 1.0f;
			ooprecision = 1.0f;
		}

		auto recall_rank_kvp(compute_recall_rank(c_found, c_suggested, required_deps, suggestions));

		return {
			{
				oocover,
				ooprecision,
				recall_rank_kvp.first,
				recall_rank_kvp.second,
				compute_auc(c_required, found_deps, irrelevant_deps, suggestions_ranks)
			},
			std::move(suggestions),
			std::move(required_deps),
			std::move(oosuggested_deps),
			std::move(oofound_deps)
		};
	}
};

}

namespace std
{

std::ostream& operator<<(std::ostream& os, roerei::performance::metrics_t const& rhs)
{
	os
		<< "100Cover " << roerei::fill(roerei::round(rhs.oocover, 3), 5) << " + "
		<< "100Precision " << roerei::fill(roerei::round(rhs.ooprecision, 3), 5) << " + "
		<< "FullRecall " << roerei::fill(roerei::round(rhs.recall, 1), 4) << " + "
		<< "Rank " << roerei::fill(roerei::round(rhs.rank, 1), 4) << " + "
		<< "AUC " << roerei::fill(roerei::round(rhs.auc, 3), 5);
	return os;
}

}
