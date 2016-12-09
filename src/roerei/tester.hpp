#pragma once

#include <roerei/dataset.hpp>
#include <roerei/storage.hpp>
#include <roerei/cv_result.hpp>

#include <roerei/ml/ml_type.hpp>
#include <roerei/ml/posetcons_type.hpp>

#include <roerei/ml/cv.hpp>
#include <roerei/ml/knn.hpp>
#include <roerei/ml/knn_adaptive.hpp>
#include <roerei/ml/naive_bayes.hpp>
#include <roerei/ml/omniscient.hpp>
#include <roerei/ml/ensemble.hpp>

#include <roerei/ml/posetcons_pessimistic.hpp>
#include <roerei/ml/posetcons_optimistic.hpp>
#include <roerei/ml/posetcons_canonical.hpp>

#include <roerei/generic/encapsulated_vector.hpp>

namespace roerei
{

class tester
{
private:
	tester() = delete;

public:
	inline static void exec(std::string const& corpus, posetcons_type strat, ml_type method, size_t jobs, bool silent=false, uint_fast32_t seed = 1337)
	{
		size_t const cv_n = 10, cv_k = 1;

		std::set<knn_params_t> ks;
		std::set<nb_params_t> nbs;
		bool run_knn_adaptive = false;
		bool run_omniscient = false;
		bool run_ensemble = false;

		switch(method)
		{
		case ml_type::knn:
			ks.emplace(knn_params_t({55}));

			for(size_t k = 3; k < 10; ++k)
				ks.emplace(knn_params_t({k}));

			for(size_t k = 10; k < 130; k+=10)
				ks.emplace(knn_params_t({k}));

			/*for(size_t k = 3; k < 120; ++k)
				ks.emplace(knn_params_t({k}));*/
			break;
		case ml_type::naive_bayes:
			/*for(size_t pi = 0; pi < 20; ++pi)
				nbs.emplace(nb_params_t({(float)pi, -15, 0}));

			for(float sigma = -20; sigma < 0; ++sigma)
				nbs.emplace(nb_params_t({10, (float)sigma, 0}));

			for(size_t tau = 0; tau < 20; ++tau)
				nbs.emplace(nb_params_t({10, -15, (float)tau}));*/

			for(float pi = 0; pi < 20; ++pi)
				for(float sigma = -20; sigma < 0; ++sigma)
					for(float tau = 0; tau < 20; ++tau)
						nbs.emplace(nb_params_t({pi, sigma, tau}));

			//nbs.emplace(nb_params_t({10, -15, 0}));
			break;
		case ml_type::knn_adaptive:
			run_knn_adaptive = true;
			break;
		case ml_type::omniscient:
			run_omniscient = true;
			break;
		case ml_type::ensemble:
			run_ensemble = true;
			break;
		}

		storage::read_result([&](cv_result_t const& result) {
			if(result.corpus != corpus)
				return;

			if(result.strat != strat)
				return;

			if(cv_n != result.n)
				return;

			if(cv_k != result.k)
				return;

			switch(result.ml)
			{
			case ml_type::knn:
				ks.erase(*result.knn_params);
				break;
			case ml_type::naive_bayes:
				nbs.erase(*result.nb_params);
				break;
			case ml_type::knn_adaptive:
				run_knn_adaptive = false;
				break;
			case ml_type::omniscient:
				run_omniscient = false;
				break;
			case ml_type::ensemble:
				run_ensemble = false;
				break;
			}

			std::cerr << "Skipping " << result << std::endl;
		});

		std::cerr << "Read results" << std::endl;

		auto const d_orig(storage::read_dataset(corpus));
		auto const d(posetcons_canonical::consistentize(d_orig, seed));
		cv const c(d, cv_n, cv_k, seed);

		std::mutex os_mutex;
		auto yield_f([&](cv_result_t const& result) {
			std::lock_guard<std::mutex> lock(os_mutex);
			storage::write_result(result);
			std::cout << result << std::endl;
		});

		std::cerr << "Scheduling..." << std::endl;

		multitask m;

		auto schedule_f([&](auto gen_trainset_sane_f) {
			auto gen_trainset_sane_f_ptr(std::make_shared<decltype(gen_trainset_sane_f)>(std::move(gen_trainset_sane_f)));

			for(knn_params_t knn_params : ks)
			{
				c.order_async(m,
					[&d, gen_trainset_sane_f_ptr, knn_params](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
						auto const trainset_sane((*gen_trainset_sane_f_ptr)(trainset, test_row));
						knn<decltype(trainset_sane)> ml(knn_params.k, trainset_sane, d);
						return performance::measure(d, test_row.row_i, ml.predict(test_row));
					},
					[=](performance::metrics_t const& total_metrics) noexcept {
						yield_f({corpus, strat, ml_type::knn, knn_params, boost::none, cv_n, cv_k, total_metrics});
					},
					d, silent
				);
			}

			if(run_knn_adaptive)
			{
				c.order_async(m,
					[&d, gen_trainset_sane_f_ptr](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
						auto const trainset_sane((*gen_trainset_sane_f_ptr)(trainset, test_row));
						knn_adaptive<decltype(trainset_sane)> ml(trainset_sane, d);
						return performance::measure(d, test_row.row_i, ml.predict(test_row));
					},
					[=](performance::metrics_t const& total_metrics) noexcept {
						yield_f({corpus, strat, ml_type::knn_adaptive, boost::none, boost::none, cv_n, cv_k, total_metrics});
					},
					d, silent
				);
			}

			if(run_omniscient)
			{
				c.order_async(m,
					[&d](cv::trainset_t const& /*trainset*/, cv::testrow_t const& test_row) noexcept {
						omniscient ml(d);
						return performance::measure(d, test_row.row_i, ml.predict(test_row));
					},
					[=](performance::metrics_t const& total_metrics) noexcept {
						yield_f({corpus, strat, ml_type::omniscient, boost::none, boost::none, cv_n, cv_k, total_metrics});
					},
					d, silent
				);
			}

			std::shared_ptr<nb_preload_data_t> nb_data;
			if(run_ensemble)
			{
				if(!nb_data)
					nb_data = std::make_shared<nb_preload_data_t>(d);

				c.order_async(m,
					[&d, gen_trainset_sane_f_ptr, nb_data](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
						auto const trainset_sane((*gen_trainset_sane_f_ptr)(trainset, test_row));
						ensemble<cv::testrow_t> e_ml(d);

						knn_adaptive<decltype(trainset_sane)> knn_ml(trainset_sane, d);
						e_ml.add_predictor([&knn_ml](auto row) {
							return knn_ml.predict(row);
						}, 0.5f);

						naive_bayes<decltype(trainset_sane)> nb_ml(
							10, -15, 0,
							d,
							*nb_data,
							trainset_sane
						);
						e_ml.add_predictor([&nb_ml, test_row_id=test_row.row_i](auto row) {
							return nb_ml.predict(row, test_row_id); // TODO remove the use of test_row_id, when time allows
						}, 0.5f);

						return performance::measure(d, test_row.row_i, e_ml.predict(test_row));
					},
					[=](performance::metrics_t const& total_metrics) noexcept {
						yield_f({corpus, strat, ml_type::ensemble, boost::none, boost::none, cv_n, cv_k, total_metrics});
					},
					d, silent
				);
			}

			for(nb_params_t const& nb_params : nbs)
			{
				if(!nb_data)
					nb_data = std::make_shared<nb_preload_data_t>(d);

				c.order_async(m,
					[&d, gen_trainset_sane_f_ptr, nb_data, nb_params](cv::trainset_t const& trainset, cv::testrow_t const& test_row) {
						auto const trainset_sane((*gen_trainset_sane_f_ptr)(trainset, test_row));
						naive_bayes<decltype(trainset_sane)> ml(
							nb_params.pi, nb_params.sigma, nb_params.tau,
							d,
							*nb_data,
							trainset_sane
						);
						return performance::measure(d, test_row.row_i, ml.predict(test_row, test_row.row_i));
					},
					[=](performance::metrics_t const& total_metrics) noexcept {
						yield_f({corpus, strat, ml_type::naive_bayes, boost::none, nb_params, cv_n, cv_k, total_metrics});
					},
					d, silent
				);
			}
		});

		switch(strat)
		{
		case posetcons_type::canonical:
		{
			schedule_f([](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
				return posetcons_canonical::exec(trainset, test_row);
			});
			break;
		}
		case posetcons_type::pessimistic:
		{
			posetcons_pessimistic pc(d);
			schedule_f([pc=std::move(pc)](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
				return pc.exec(trainset, test_row.row_i);
			});
			break;
		}
		case posetcons_type::optimistic:
		{
			posetcons_optimistic pc(d);
			schedule_f([pc=std::move(pc)](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
				return pc.exec(trainset, test_row.row_i);
			});
			break;
		}
		}

		std::cerr << "Scheduled" << std::endl;

		m.run(jobs, true); // Blocking
		std::cerr << "Finished" << std::endl;
	}
};

}
