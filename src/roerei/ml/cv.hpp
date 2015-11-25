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
private:
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
		encapsulated_vector<object_id_t, std::vector<object_id_t>> dependants_real;
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

		std::map<dependency_id_t, object_id_t> dependency_map(d.create_dependency_map());
		dependencies::dependant_obj_matrix_t dependants_trans(dependencies::create_obj_dependants(d, dependency_map));
		dependants_trans.transitive();

		decltype(static_t::dependants_real) dependants_real(dependants_trans.size_m());
		d.objects.keys([&](object_id_t i) {
			std::set<object_id_t> const& objs = dependants_trans[i];
			dependants_real[i].insert(dependants_real[i].end(), objs.begin(), objs.end());
		});

		std::vector<size_t> partitions(n);
		std::iota(partitions.begin(), partitions.end(), 0);

		/* Prepackage static cross-validation data */
		cv_static_ptr = std::make_shared<static_t>(static_t{
			std::move(partitions),
			partition::generate_bare(d.objects.size(), n, seed_opt),
			std::move(dependants_real),
			d.feature_matrix
		});
	}

	typedef bl_sparse_matrix_t<compact_sparse_matrix_t<object_id_t, feature_id_t, dataset_t::value_t> const> trainset_t;
	typedef compact_sparse_matrix_t<object_id_t, feature_id_t, dataset_t::value_t>::const_row_proxy_t testrow_t;
	typedef std::function<performance::result_t(trainset_t const&, testrow_t const&)> ml_f_t;

	template<typename ML_F, typename RESULT_F>
	void order_async(multitask& m, ML_F const& ml_f, RESULT_F const& result_f, dataset_t const& d, bool silent = false) const
	{
		std::vector<std::packaged_task<void()>> tasks;
		std::vector<std::future<performance::metrics_t>> future_metrics;

		size_t i = 0;
		combs(n, n-k, [&](std::vector<size_t> const& train_ps) {
			std::promise<performance::metrics_t> p;
			future_metrics.emplace_back(p.get_future());

			tasks.emplace_back([&d, ml_f, silent, i, train_ps, cv_static_ptr=this->cv_static_ptr, p=std::move(p)]() mutable {
				auto const& s = *cv_static_ptr;

				test::performance::init();

				sliced_sparse_matrix_t<decltype(s.feature_matrix) const> train_m_tmp(s.feature_matrix, false), test_m_tmp(s.feature_matrix, false);
				d.objects.keys([&](object_id_t j) {
					if(std::binary_search(train_ps.begin(), train_ps.end(), s.partition_subdivision[j.unseal()]))
						train_m_tmp.add_key(j);
					else
						test_m_tmp.add_key(j);
				});

				compact_sparse_matrix_t<object_id_t, feature_id_t, dataset_t::value_t> const train_m(train_m_tmp), test_m(test_m_tmp);

				performance::metrics_t fm;

				{
					performance_scope("citerate")
					test_m.citerate([&](testrow_t const& test_row) {
						trainset_t train_m_sane(train_m, s.dependants_real[test_row.row_i]);
						fm += ml_f(train_m_sane, test_row).metrics;
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

			for(auto& m : future_metrics)
				total_metrics += m.get();

			result_f(total_metrics);
		});

		m.add({
			std::move(tasks),
			std::move(continuation)
		});
	}
};

}
