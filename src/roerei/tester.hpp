#pragma once

#include <roerei/dataset.hpp>
#include <roerei/cv_result.hpp>

#include <roerei/ml/cv.hpp>
#include <roerei/ml/knn.hpp>
#include <roerei/ml/naive_bayes.hpp>

#include <roerei/generic/encapsulated_vector.hpp>

namespace roerei
{

class tester
{
private:
	tester() = delete;

	struct nb_data_t
	{
		nb_data_t(nb_data_t const&) = delete;
		nb_data_t(nb_data_t&&) = default;

		dependencies::dependant_matrix_t dependants;
		encapsulated_vector<object_id_t, std::vector<dependency_id_t>> allowed_dependencies;
		encapsulated_vector<feature_id_t, std::vector<object_id_t>> feature_occurance;
	};

	inline static nb_data_t load_nb_data(dataset_t const& d)
	{
		std::map<object_id_t, dependency_id_t> dependency_revmap(d.create_dependency_revmap());
		dependencies::dependant_matrix_t dependants(dependencies::create_dependants(d));
		dependencies::dependant_obj_matrix_t dependants_trans(dependencies::create_obj_dependants(d));
		dependants_trans.transitive();

		encapsulated_vector<object_id_t, std::vector<dependency_id_t>> allowed_dependencies;
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

		encapsulated_vector<feature_id_t, std::vector<object_id_t>> feature_occurance(d.features.size());
		d.feature_matrix.citerate([&](dataset_t::feature_matrix_t::const_row_proxy_t const& row) {
			for(auto const& kvp : row)
			{
				assert(kvp.second > 0);
				feature_occurance[kvp.first].emplace_back(row.row_i);
			}
		});

		return {
			std::move(dependants),
			std::move(allowed_dependencies),
			std::move(feature_occurance)
		};
	}

public:
	inline static void exec(std::string const& corpus, size_t jobs, bool silent=false)
	{
		std::string const knn_str = "knn", nb_str = "nb";
		size_t const n = 10, k = 3;

		std::set<knn_params_t> ks;
		std::set<nb_params_t> nbs;

		for(size_t k = 3; k < 120; ++k)
			ks.emplace(knn_params_t({k}));

		nbs.emplace(nb_params_t({10, -15, 20}));

		storage::read_result([&](cv_result_t const& result) {
			if(result.corpus != corpus)
				return;

			if(result.ml == knn_str)
				ks.erase(*result.knn_params);
			else if(result.ml == nb_str)
				nbs.erase(*result.nb_params);
			else
				throw std::runtime_error(std::string("Unknown ml method ") + result.ml);

			std::cerr << "Skipping " << result << std::endl;
		});

		std::cerr << "Read results" << std::endl;

		auto const d(storage::read_dataset(corpus));
		cv const c(d, n, k, 1337);

		std::mutex os_mutex;
		std::ofstream os("./data/results.msgpack", std::ios::app | std::ios::out | std::ios::binary);

		auto yield_f([&](cv_result_t const& result) {
			std::lock_guard<std::mutex> lock(os_mutex);
			msgpack_serializer s;
			serialize(s, "cv_result", result);
			s.dump([&os](const char* buf, size_t len) {
				os.write(buf, len);
				os.flush();
			});

			std::cout << result << std::endl;
		});

		multitask m;
		for(knn_params_t knn_params : ks)
		{
			c.order_async(m,
				[knn_params, &d](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
					knn<cv::trainset_t const> ml(knn_params.k, trainset, d);
					return performance::measure(d, test_row.row_i, ml.predict(test_row));
				},
				[=](performance::metrics_t const& total_metrics) noexcept {
					yield_f({corpus, knn_str, knn_params, boost::none, n, k, total_metrics});
				},
				d, silent
			);
		}

		std::shared_ptr<nb_data_t> nb_data;
		for(nb_params_t const& nb_params : nbs)
		{
			if(!nb_data)
				nb_data = std::make_shared<nb_data_t>(std::move(load_nb_data(d)));

			c.order_async(m,
				[&d, nb_data, nb_params](cv::trainset_t const& trainset, cv::testrow_t const& test_row) {
					naive_bayes<decltype(trainset)> ml(
						nb_params.pi, nb_params.sigma, nb_params.tau,
						d,
						nb_data->feature_occurance,
						nb_data->dependants,
						nb_data->allowed_dependencies[test_row.row_i],
						trainset
					);
					return performance::measure(d, test_row.row_i, ml.predict(test_row));
				},
				[=](performance::metrics_t const& total_metrics) noexcept {
					yield_f({corpus, nb_str, boost::none, nb_params, n, k, total_metrics});
				},
				d, silent
			);
		}

		m.run(jobs, true); // Blocking
		std::cerr << "Finished" << std::endl;
	}
};

}
