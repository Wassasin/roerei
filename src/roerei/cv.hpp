#pragma once

#include <roerei/performance.hpp>
#include <roerei/knn.hpp>
#include <roerei/partition.hpp>
#include <roerei/dataset.hpp>
#include <roerei/create_map.hpp>
#include <roerei/sliced_sparse_matrix.hpp>
#include <roerei/compact_sparse_matrix.hpp>
#include <roerei/sparse_unit_matrix.hpp>
#include <roerei/bl_sparse_matrix.hpp>

#include <iostream>

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

	static sparse_unit_matrix_t create_dependants(dataset_t const& d)
	{
		std::map<uri_t, size_t> objects_map(create_map<uri_t>(d.objects));
		sparse_unit_matrix_t result(d.objects.size(), d.objects.size());

		d.dependency_matrix.iterate([&](dataset_t::matrix_t::const_row_proxy_t const& xs) {
			for(auto const& kvp : xs)
			{
				size_t dep_i = objects_map[d.dependencies[kvp.first]];

				if(kvp.second > 0)
					result.set(std::make_pair(dep_i, xs.row_i));
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
		for(size_t const j : dependants[i])
			_iterate_dependants_helper(dependants, j, yield, visited);
	}

	template<typename MATRIX, typename F>
	static void iterate_dependants(MATRIX const& dependants, size_t i, F const& yield)
	{
		std::set<size_t> visited;
		_iterate_dependants_helper(dependants, i, yield, visited);
	}

public:
	static inline void exec(dataset_t const& d, size_t const n, size_t const k = 1, boost::optional<uint_fast32_t> seed_opt = boost::none)
	{
		assert(n >= k);

		sparse_unit_matrix_t dependants(create_dependants(d));
		dependants.transitive();

		std::vector<std::vector<size_t>> dependants_real(dependants.size_m());
		for(size_t i = 0; i < dependants.size_m(); i++)
			dependants_real[i].insert(dependants_real[i].end(), dependants[i].begin(), dependants[i].end());

		compact_sparse_matrix_t<dataset_t::value_t> feature_matrix(d.feature_matrix);

		uint_fast32_t seed = seed_opt ? *seed_opt : 1337;
		std::vector<size_t> partition_subdivision(partition::generate_bare(d.objects.size(), n, seed));
		std::vector<size_t> partitions(n);
		std::iota(partitions.begin(), partitions.end(), 0);

		size_t i = 0;
		combs(n, n-k, [&](std::vector<size_t> const& train_ps) {
			std::vector<size_t> test_ps;
			std::set_difference(
				partitions.begin(), partitions.end(),
				train_ps.begin(), train_ps.end(),
				std::inserter(test_ps, test_ps.begin())
			);

			sliced_sparse_matrix_t<decltype(feature_matrix) const> train_m_tmp(feature_matrix, false), test_m_tmp(feature_matrix, false);
			for(size_t j = 0; j < d.objects.size(); ++j)
			{
				size_t p = partition_subdivision[j];
				if(std::binary_search(test_ps.begin(), test_ps.end(), p))
					test_m_tmp.add_key(j);
				else
					train_m_tmp.add_key(j);
			}
			compact_sparse_matrix_t<dataset_t::value_t> const train_m(train_m_tmp), test_m(test_m_tmp);

			size_t const test_m_size = test_m_tmp.nonempty_size_m();
			float avgoocover = 0.0f, avgooprecision = 0.0f;
			size_t j = 0;
			test_m.citerate([&](decltype(feature_matrix)::const_row_proxy_t const& test_row) {
				bl_sparse_matrix_t<decltype(feature_matrix) const> train_m_sane(train_m, dependants_real[test_row.row_i]);

				knn<std::remove_reference<decltype(train_m_sane)>::type> c(5, train_m_sane);
				performance::result_t r(performance::measure(d, c, test_row));

				avgoocover += r.oocover;
				avgooprecision += r.ooprecision;

				if(j % (test_m_size / 200) == 0)
				{
					float percentage = round((float)j / (float)test_m_size * 100.0f, 2);
					std::cout << '\r' << i << ": " << fill(percentage, 5) << '%';
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
