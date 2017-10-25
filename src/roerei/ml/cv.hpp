#pragma once

#include <roerei/performance.hpp>
#include <roerei/partition.hpp>
#include <roerei/dataset.hpp>
#include <roerei/dependencies.hpp>

#include <roerei/generic/multitask.hpp>

#include <roerei/generic/sliced_sparse_matrix.hpp>
#include <roerei/generic/compact_sparse_matrix.hpp>
#include <roerei/generic/bl_sparse_matrix.hpp>

#include <roerei/util/performance.hpp>

#include <iostream>
#include <functional>

namespace roerei
{

class cv
{
public:
	static const size_t default_n = 10;
	static const size_t default_k = 1;
private:
	struct partition_object_id_t : public id_t<partition_object_id_t> {
		partition_object_id_t(size_t id)
		: id_t<partition_object_id_t>(id)
		{
			//
		}
	};

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
		encapsulated_vector<partition_object_id_t, size_t> partition_subdivision;
		encapsulated_vector<partition_object_id_t, object_id_t> partition_map;
		compact_sparse_matrix_t<object_id_t, feature_id_t, dataset_t::value_t> feature_matrix;
	};

	size_t n, k;
	std::shared_ptr<static_t const> cv_static_ptr;

public:
	cv(dataset_t const& d, size_t const _n, size_t const _k = 1, boost::optional<uint_fast32_t> seed_opt = boost::none)
	: n(_n)
	, k(_k)
	, cv_static_ptr()
	{
		assert(n >= k);

		std::vector<size_t> partitions(n);
		std::iota(partitions.begin(), partitions.end(), 0);

		size_t new_size = d.objects.size() - d.prior_objects.size();
		decltype(static_t::partition_map) partition_map;
		partition_map.reserve(new_size);
		object_id_t::iterate([&](object_id_t id) {
			if (d.prior_objects.find(id) != d.prior_objects.end()) {
				return;
			}
			partition_map.emplace_back(id);
		}, d.objects.size());

		decltype(static_t::partition_subdivision) partition_subdivision;
		partition_subdivision.reserve(new_size);
		for (size_t partition : partition::generate_bare(new_size, n, seed_opt)) {
			partition_subdivision.emplace_back(partition);
		}

		/* Prepackage static cross-validation data */
		cv_static_ptr = std::make_shared<static_t>(static_t{
			std::move(partitions),
			std::move(partition_subdivision),
			std::move(partition_map),
			d.feature_matrix
		});
	}

	typedef compact_sparse_matrix_t<object_id_t, feature_id_t, dataset_t::value_t> const trainset_t;
	typedef compact_sparse_matrix_t<object_id_t, feature_id_t, dataset_t::value_t>::const_row_proxy_t testrow_t;
	typedef std::function<performance::result_t(trainset_t const&, testrow_t const&)> ml_f_t;

	template<typename ML_F, typename RESULT_F>
	void order_async(multitask& m, ML_F const& init_f, RESULT_F const& result_f, dataset_t const& d, bool prior = true, bool silent = false) const
	{
		std::vector<std::packaged_task<void()>> tasks;
		std::vector<std::future<performance::metrics_t>> future_metrics;

		size_t i = 0;
		combs(n, n-k, [&](std::vector<size_t> const& train_ps) {
			std::promise<performance::metrics_t> p;
			future_metrics.emplace_back(p.get_future());

			tasks.emplace_back([&d, init_f, prior, silent, i, train_ps, cv_static_ptr=this->cv_static_ptr, p=std::move(p), n=n]() mutable {
				auto const& s = *cv_static_ptr;

				test::performance::init();

				sliced_sparse_matrix_t<decltype(s.feature_matrix) const> train_m_tmp(s.feature_matrix, false), test_m_tmp(s.feature_matrix, false);
				if (n == 1) {
					// HACK; implements a faux non-cv mode
					object_id_t::iterate([&](object_id_t j) {
						train_m_tmp.add_key(j);
					}, d.objects.size());
				} else if (prior) {
					for(object_id_t j : d.prior_objects) {
						train_m_tmp.add_key(j);
					}
				}

				s.partition_map.iterate([&](partition_object_id_t j_partition, object_id_t j) {
					if(std::binary_search(train_ps.begin(), train_ps.end(), s.partition_subdivision[j_partition]))
						train_m_tmp.add_key(j);
					else
						test_m_tmp.add_key(j);
				});

				compact_sparse_matrix_t<object_id_t, feature_id_t, dataset_t::value_t> const train_m(train_m_tmp), test_m(test_m_tmp);
				std::cerr << "prior " << d.prior_objects.size() << std::endl;
				{
					size_t c = 0;
					train_m.citerate([&c](auto) {
						c++;
					});
					std::cerr << "train_m_tmp " << c << std::endl;
				}
				{
					size_t c = 0;
					test_m.citerate([&c](auto) {
						c++;
					});
					std::cerr << "test_m_tmp " << c << std::endl;
				}

				performance::metrics_t fm;

				{
					performance_scope("citerate")
					auto&& ml_f = init_f(train_m);
					test_m.citerate([&](testrow_t const& test_row) {
						fm += ml_f(test_row).metrics;
					});
				}

				if(!silent)
				{
					std::cout << i << ": " << fm << std::endl;
					test::performance::init().report();
				}

				test::performance::clear();

				p.set_value(std::move(fm));
			});
			i++;
		});

		std::packaged_task<void()> continuation([future_metrics=std::move(future_metrics), result_f]() mutable
		{
			performance::metrics_t total_metrics;

			for(auto& fut_m : future_metrics)
				total_metrics += fut_m.get();

			result_f(total_metrics);
		});

		m.add({
			std::move(tasks),
			std::move(continuation)
		});
	}
};

}
