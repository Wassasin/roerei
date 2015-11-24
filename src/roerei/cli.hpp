#pragma once

#include <roerei/ml/knn.hpp>
#include <roerei/ml/naive_bayes.hpp>
#include <roerei/ml/cv.hpp>

#include <roerei/generic/multitask.hpp>

#include <roerei/generator.hpp>
#include <roerei/performance.hpp>

#include <roerei/inspector.hpp>

#include <iostream>
#include <boost/program_options.hpp>

namespace roerei
{

class cli
{
private:
	struct cli_options
	{
		std::string action;
		std::string corpus;
		bool silent = false;
		size_t jobs = 1;
	};

	static int read_options(cli_options& opt, int argc, char** argv)
	{
		boost::program_options::options_description o_general("Options");
		o_general.add_options()
				("help,h", "display this message")
				("silent,s", "do not print progress")
				("corpus,c", boost::program_options::value(&opt.corpus), "select which corpus to sample or generate (default: Coq)")
				("jobs,j", boost::program_options::value(&opt.jobs), "number of concurrent jobs (default: 1)");

		boost::program_options::variables_map vm;
		boost::program_options::positional_options_description pos;

		pos.add("action", 1);

		boost::program_options::options_description options("Allowed options");
		options.add(o_general);
		options.add_options()
				("action", boost::program_options::value(&opt.action));

		try
		{
			boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options).positional(pos).run(), vm);
		} catch(boost::program_options::unknown_option &e)
		{
			std::cerr << "Unknown option --" << e.get_option_name() << ", see --help." << std::endl;
			return EXIT_FAILURE;
		}

		try
		{
			boost::program_options::notify(vm);
		} catch(const boost::program_options::required_option &e)
		{
			std::cerr << "You forgot this: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}

		if(vm.count("help"))
		{
			std::cout
					<< "Premise selection for Coq in OCaml/C++. [https://github.com/Wassasin/roerei]" << std::endl
					<< "Usage: ./roerei [options] action" << std::endl
					<< std::endl
					<< "Actions:" << std::endl
					<< "  generate       load repo.msgpack, convert and write to dataset.msgpack" << std::endl
					<< "  cv             test performance using cross-validation" << std::endl
					<< "  test           placeholder action" << std::endl
					<< std::endl
					<< o_general;

			return EXIT_FAILURE;
		}

		if(vm.count("silent"))
			opt.silent = true;

		if(!vm.count("action"))
		{
			std::cerr << "Please specify an action, see --help." << std::endl;
			return EXIT_FAILURE;
		}

		if(!vm.count("corpus"))
		{
			opt.corpus = "Coq";
		}

		return EXIT_SUCCESS;
	}

public:
	cli() = delete;

	static int exec(int argc, char** argv)
	{
		cli_options opt;

		int result = read_options(opt, argc, argv);
		if(result != EXIT_SUCCESS)
			return result;

		if(opt.action == "test")
		{
			auto const d(storage::read_dataset(opt.corpus));
			inspector::iterate_all(d);
		}
		else if(opt.action == "cv-knn")
		{
			auto const d(storage::read_dataset(opt.corpus));

			std::vector<size_t> ks;
			std::vector<cv::ml_f_t> ml_fs;
			for(size_t k = 60; k < 61; ++k)
			{
				ks.emplace_back(k);
				ml_fs.emplace_back([k, &d](cv::trainset_t const& trainset, cv::testrow_t const& test_row) noexcept {
					knn<cv::trainset_t const> ml(k, trainset, d);
					return performance::measure(d, test_row.row_i, ml.predict(test_row));
				});
			}

			multitask m;
			auto futures = cv::order_async_mult(m, ml_fs, d, 10, 3, opt.silent, 1337);
			m.run(opt.jobs, false);

			for(size_t i = 0; i < ks.size(); ++i)
			{
				auto total_metrics(futures.at(i).get());
				std::cout << "K=" << ks[i] << " - " << total_metrics << std::endl;
			}

			std::cerr << "Finished" << std::endl;
		}
		else if(opt.action == "cv-nb")
		{
			auto const d(storage::read_dataset(opt.corpus));

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

			multitask m;
			size_t i = 0;
			auto future = cv::order_async(m, [&i, &d, &feature_occurance, &dependants, &allowed_dependencies](cv::trainset_t const& trainset, cv::testrow_t const& test_row) {
					naive_bayes<decltype(trainset)> ml(d, feature_occurance, dependants, allowed_dependencies[test_row.row_i], trainset);
					return performance::measure(d, test_row.row_i, ml.predict(test_row));
			}, d, 10, 3, opt.silent, 1337);

			std::cerr << "Initialized" << std::endl;

			//m.run(opt.jobs, false);
			m.run_synced();

			std::cout << future.get() << std::endl;
		}
		else if(opt.action == "generate")
		{
			std::map<std::string, dataset_t> map(generator::construct_from_repo());
			for(auto const& kvp : map)
			{
				storage::write_dataset(kvp.first, kvp.second);
				std::cerr << "Written " << kvp.first << std::endl;
			}
		}
		else
		{
			std::cerr << "Unknown action '" << opt.action << "', see --help." << std::endl;
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

};

}
