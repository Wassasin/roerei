#pragma once

#include <roerei/knn.hpp>
#include <roerei/partition.hpp>
#include <roerei/dataset.hpp>

#include <roerei/sliced_sparse_matrix.hpp>

namespace roerei
{

class cv
{
private:
	cv() = delete;

	template<typename F>
	static void _combs_helper(size_t const n, size_t const k, F const& yield, size_t offset, std::vector<size_t>& buf)
	{
		if(k == 0)
		{
			yield(buf);
			return;
		}

		for(size_t i = offset; i <= n - k; ++i)
		{
			buf.emplace_back(i);
			_combs_helper(n, k-1, yield, i+1, buf);
			buf.pop_back();
		}
	}

	template<typename F>
	static void combs(size_t const n, size_t const k, F const& yield)
	{
		std::vector<size_t> buf;
		buf.reserve(k);
		_combs_helper(n, k, yield, 0, buf);
	}

public:
	static inline void exec(dataset_t const& d, size_t const n, size_t const k = 1)
	{
		assert(n >= k);

		std::vector<size_t> partition_subdivision(partition::generate_bare(d.objects.size(), n));
		std::vector<size_t> partitions(n);
		std::iota(partitions.begin(), partitions.end(), 0);

		size_t i = 0;
		combs(n, (n-k), [&](std::vector<size_t> const& train_ps) {
			std::vector<size_t> test_ps;
			std::set_difference(
				partitions.begin(), partitions.end(),
				train_ps.begin(), train_ps.end(),
				std::inserter(test_ps, test_ps.begin())
			);

			sliced_sparse_matrix_t<decltype(d.feature_matrix) const> train_m(d.feature_matrix), test_m(d.feature_matrix);
			for(size_t j = 0; j < d.objects.size(); ++j)
			{
				size_t p = partition_subdivision[j];
				if(std::binary_search(test_ps.begin(), test_ps.end(), p))
					test_m.add_key(j);
				else
					train_m.add_key(j);
			}

			std::cout << "Size: " << train_m.size() << "+" << test_m.size() << std::endl;

			knn<decltype(train_m)> c(5, train_m);

			std::map<size_t, float> suggestions; // <id, weighted freq>

			size_t j = 0;
			for(auto const& test_row : test_m)
			{
				auto predictions(c.predict(test_row));
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

				std::set<size_t> required_deps, suggested_deps, found_deps, missing_deps;
				for(auto const& kvp : d.dependency_matrix[test_row.row_i])
					required_deps.insert(kvp.first);

				for(size_t j = 0; j < suggestions_sorted.size() && j < 100; ++j)
					suggested_deps.insert(suggestions_sorted[j].first);

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

				float c_required = required_deps.size();
				float c_suggested = suggested_deps.size();
				float c_found = found_deps.size();

				float oocover = c_found/c_required;
				float ooprecision = c_found/c_suggested;

				if(required_deps.empty())
				{
					oocover = 1.0f;
					ooprecision = 1.0f;
				}

				std::cout << i << " - " << j << " [" << test_row.row_i << "]: " << oocover << std::endl;
				j++;
			}
			i++;
		});
	}
};

}
