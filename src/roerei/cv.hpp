#pragma once

#include <roerei/performance.hpp>
#include <roerei/knn.hpp>
#include <roerei/partition.hpp>
#include <roerei/dataset.hpp>
#include <roerei/create_map.hpp>
#include <roerei/sliced_sparse_matrix.hpp>
#include <roerei/compact_sparse_matrix.hpp>

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

	static dataset_t::matrix_t create_dependants(dataset_t const& d)
	{
		std::map<uri_t, size_t> objects_map(create_map<uri_t>(d.objects));
		dataset_t::matrix_t result(d.objects.size(), d.objects.size());

		d.dependency_matrix.iterate([&](dataset_t::matrix_t::const_row_proxy_t const& xs) {
			for(auto const& kvp : xs)
			{
				size_t dep_i = objects_map[d.dependencies[kvp.first]];
				result[dep_i][xs.row_i] = kvp.second;
			}
		});
		return result;
	}

	template<typename MATRIX, typename F>
	static void _iterate_dependants_helper(MATRIX const& dependants, size_t i, F const& yield, std::set<size_t>& visited)
	{
		if(!visited.emplace(i).second)
			return;

		yield(i);
		for(auto const& kvp : dependants[i])
			_iterate_dependants_helper(dependants, kvp.first, yield, visited);
	}

	template<typename MATRIX, typename F>
	static void iterate_dependants(MATRIX const& dependants, size_t i, F const& yield)
	{
		std::set<size_t> visited;
		_iterate_dependants_helper(dependants, i, yield, visited);
	}

public:
	static inline void exec(dataset_t const& d, size_t const n, size_t const k = 1)
	{
		assert(n >= k);

		compact_sparse_matrix_t<dataset_t::value_t> dependants(create_dependants(d));
		compact_sparse_matrix_t<dataset_t::value_t> feature_matrix(d.feature_matrix);

		std::vector<size_t> partition_subdivision(partition::generate_bare(d.objects.size(), n, 1337));
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

			sliced_sparse_matrix_t<decltype(feature_matrix) const> train_m(feature_matrix, false), test_m(feature_matrix, false);
			for(size_t j = 0; j < d.objects.size(); ++j)
			{
				size_t p = partition_subdivision[j];
				if(std::binary_search(test_ps.begin(), test_ps.end(), p))
					test_m.add_key(j);
				else
					train_m.add_key(j);
			}

			size_t const test_m_size = test_m.size();
			float avgoocover = 0.0f, avgooprecision = 0.0f;
			size_t j = 0;
			test_m.iterate([&](decltype(feature_matrix)::const_row_proxy_t const& test_row) {
				sliced_sparse_matrix_t<decltype(feature_matrix) const> train_m_sane(train_m);
				iterate_dependants(dependants, test_row.row_i, [&](size_t i) {
					train_m_sane.try_remove_key(i);
				});

				knn<decltype(train_m)> c(5, train_m_sane);
				performance::result_t r(performance::measure(d, c, test_row));

				avgoocover += r.oocover;
				avgooprecision += r.ooprecision;

				if(j % 10 == 0)
				{
					float percentage = (float)j / (float)test_m_size * 100.0f;
					std::cout << '\r' << i << ": " << percentage << '%';
					std::cout.flush();
				}

				j++;
			});

			std::cout << '\r';
			std::cout.flush();

			avgoocover /= (float)j;
			avgooprecision /= (float)j;

			std::cout << i << ": " << avgoocover << " + " << avgooprecision << std::endl;

			i++;
		});
	}
};

}
