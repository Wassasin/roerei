#pragma once

#include <roerei/dataset.hpp>
#include <roerei/math.hpp>

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
		std::vector<std::pair<size_t, float>> predictions;
		std::vector<std::pair<size_t, float>> suggestions_sorted;
		std::set<size_t> required_deps, oosuggested_deps, oofound_deps, missing_deps;
	};

	template<typename ML, typename ROW>
	static result_t measure(dataset_t const& d, ML const& ml, ROW const& test_row)
	{
		std::map<size_t, float> suggestions; // <id, weighted freq>

		auto predictions(ml.predict(test_row));
		std::reverse(predictions.begin(), predictions.end());
		for(auto const& kvp : predictions)
		{
			float weight = 1.0f / (kvp.second + 1.0f); // "Similarity", higher is more similar

			for(auto dep_kvp : d.dependency_matrix[kvp.first])
				suggestions[dep_kvp.first] += ((float)dep_kvp.second) * weight;
		}

		std::vector<std::pair<size_t, float>> suggestions_sorted;
		for(auto const& kvp : suggestions)
			suggestions_sorted.emplace_back(kvp);

		std::sort(suggestions_sorted.begin(), suggestions_sorted.end(), [&](std::pair<size_t, float> const& x, std::pair<size_t, float> const& y) {
			return x.second > y.second;
		});

		std::map<size_t, size_t> suggestions_ranks;
		for(size_t j = 0; j < suggestions_sorted.size() && j < 100; ++j)
			suggestions_ranks[suggestions_sorted[j].first] = j;

		std::set<size_t> required_deps, suggested_deps, oosuggested_deps, found_deps, oofound_deps, missing_deps, irrelevant_deps;
		for(auto const& kvp : d.dependency_matrix[test_row.row_i])
			required_deps.insert(kvp.first);

		for(size_t j = 0; j < suggestions_sorted.size() && j < 100; ++j)
			oosuggested_deps.insert(suggestions_sorted[j].first);

		for(size_t j = 0; j < suggestions_sorted.size(); ++j)
			suggested_deps.insert(suggestions_sorted[j].first);

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
			required_deps.begin(), required_deps.end(),
			found_deps.begin(), found_deps.end(),
			std::inserter(missing_deps, missing_deps.begin())
		);

		std::set_difference(
			suggested_deps.begin(), suggested_deps.end(),
			found_deps.begin(), found_deps.end(),
			std::inserter(irrelevant_deps, irrelevant_deps.begin())
		);

		float c_required = required_deps.size();
		float c_found = found_deps.size();
		float c_suggested = suggested_deps.size();
		float c_irrelevant = irrelevant_deps.size();
		float c_oosuggested = oosuggested_deps.size();
		float c_oofound = oofound_deps.size();

		float oocover = c_oofound/c_required;
		float ooprecision = c_oofound/c_oosuggested;
		float recall = c_suggested + 1.0f;
		float rank = 0.0f;
		float auc = 0.0f;

		if(c_oofound == 0.0f)
			ooprecision = 0.0f;

		if(c_required == 0.0f)
		{
			oocover = 1.0f;
			ooprecision = 1.0f;
		}

		if(missing_deps.empty())
		{
			std::set<size_t> todo_deps(required_deps); // Copy
			size_t j = 0;
			for(; j < suggestions_sorted.size() && !todo_deps.empty(); ++j)
				if(todo_deps.erase(suggestions_sorted[j].first) > 0)
					rank += j;

			recall = j;
			rank /= c_found;
		}

		if(c_found == 0)
			rank = c_suggested + 1.0f;

		for(size_t found_dep : found_deps)
			for(size_t irrelevant_dep : irrelevant_deps)
				if(suggestions_ranks[found_dep] > suggestions_ranks[irrelevant_dep])
					auc += 1.0f;

		if(c_found * c_irrelevant != 0)
			auc /= c_found * c_irrelevant;
		else if(c_irrelevant == 0)
			auc = 1.0f;
		else
			auc = 0.0f;

		return {
			{
				oocover,
				ooprecision,
				recall,
				rank,
				auc
			},
			std::move(predictions),
			std::move(suggestions_sorted),
			std::move(required_deps),
			std::move(oosuggested_deps),
			std::move(oofound_deps),
			std::move(missing_deps)
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
