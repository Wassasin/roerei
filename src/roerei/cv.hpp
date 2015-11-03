#pragma once

#include <roerei/performance.hpp>
#include <roerei/knn.hpp>
#include <roerei/partition.hpp>
#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>
#include <roerei/multitask.hpp>

#include <roerei/sliced_sparse_matrix.hpp>
#include <roerei/compact_sparse_matrix.hpp>
#include <roerei/bl_sparse_matrix.hpp>

#include <iostream>
#include <functional>

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

	struct static_t
	{
		std::vector<size_t> partitions;
		std::vector<size_t> partition_subdivision;
		std::vector<std::vector<size_t>> dependants_real;
		compact_sparse_matrix_t<dataset_t::value_t> feature_matrix;
	};

public:
	typedef bl_sparse_matrix_t<compact_sparse_matrix_t<dataset_t::value_t> const> trainset_t;
	typedef compact_sparse_matrix_t<dataset_t::value_t>::const_row_proxy_t testrow_t;
	typedef std::function<performance::result_t(dataset_t const&, trainset_t const&, testrow_t const&)> ml_f_t;

	template<typename CONTAINER>
	static inline std::vector<std::future<performance::metrics_t>> order_async_mult(multitask& m, CONTAINER const& ml_fs, dataset_t const& d, size_t const n, size_t const k = 1, bool silent = false, boost::optional<uint_fast32_t> seed_opt = boost::none)
	{
		assert(n >= k);

		sparse_unit_matrix_t dependants(dependencies::create_dependants(d));
		dependants.transitive();

		std::vector<std::vector<size_t>> dependants_real(dependants.size_m());
		for(size_t i = 0; i < dependants.size_m(); i++)
			dependants_real[i].insert(dependants_real[i].end(), dependants[i].begin(), dependants[i].end());

		std::vector<size_t> partitions(n);
		std::iota(partitions.begin(), partitions.end(), 0);

		/* Prepackage static cross-validation data */
		std::shared_ptr<static_t const> cv_static_ptr(std::make_shared<static_t>(static_t{
			std::move(partitions),
			partition::generate_bare(d.objects.size(), n, seed_opt),
			std::move(dependants_real),
			compact_sparse_matrix_t<dataset_t::value_t>(d.feature_matrix)
		}));

		std::vector<std::future<performance::metrics_t>> result;
		for(auto const& ml_f : ml_fs)
		{
			std::vector<std::packaged_task<void()>> tasks;
			std::vector<std::future<performance::metrics_t>> future_metrics;

			size_t i = 0;
			combs(n, n-k, [&](std::vector<size_t> const& train_ps) {
				std::promise<performance::metrics_t> p;
				future_metrics.emplace_back(p.get_future());

				tasks.emplace_back([&d, ml_f, silent, i, train_ps, cv_static_ptr, p=std::move(p)]() mutable {
					auto const& s = *cv_static_ptr;

					sliced_sparse_matrix_t<decltype(s.feature_matrix) const> train_m_tmp(s.feature_matrix, false), test_m_tmp(s.feature_matrix, false);
					for(size_t j = 0; j < d.objects.size(); ++j)
					{
						if(std::binary_search(train_ps.begin(), train_ps.end(), s.partition_subdivision[j]))
							train_m_tmp.add_key(j);
						else
							test_m_tmp.add_key(j);
					}

					compact_sparse_matrix_t<dataset_t::value_t> const train_m(train_m_tmp), test_m(test_m_tmp);

					performance::metrics_t fm;
					test_m.citerate([&](testrow_t const& test_row) {
						trainset_t train_m_sane(train_m, s.dependants_real[test_row.row_i]);
						fm += ml_f(d, train_m_sane, test_row).metrics;
					});

					if(!silent)
					{
						std::cout << i << ": " << fill(fm.oocover, 8) << " + " << fill(fm.ooprecision, 8) << " + " << fm.recall << std::endl;
					}

					p.set_value(std::move(fm));
				});
				i++;
			});

			std::promise<performance::metrics_t> result_promise;
			result.emplace_back(result_promise.get_future());

			std::packaged_task<void()> continuation([future_metrics=std::move(future_metrics), result_promise=std::move(result_promise)]() mutable
			{
				performance::metrics_t total_metrics;

				for(auto& m : future_metrics)
					total_metrics += m.get();

				result_promise.set_value(total_metrics);
			});

			m.add({
				std::move(tasks),
				std::move(continuation)
			});
		}

		return result;
	}

	template<typename F>
	static inline std::future<performance::metrics_t> order_async(multitask& m, F const& ml_f, dataset_t const& d, size_t const n, size_t const k = 1, bool silent = false, boost::optional<uint_fast32_t> seed_opt = boost::none)
	{
		std::initializer_list<F> ml_fs({{ml_f}});
		return std::move(order_async_mult(m, ml_fs, d, n, k, silent, seed_opt).front());
	}
};

}
