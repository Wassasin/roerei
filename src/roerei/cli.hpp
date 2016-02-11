#pragma once

#include <roerei/generator.hpp>
#include <roerei/inspector.hpp>
#include <roerei/tester.hpp>

#include <roerei/ml/ml_type.hpp>

#include <iostream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace roerei
{

class cli
{
private:
	struct cli_options
	{
		std::string action;
		std::string corpii;
		std::vector<ml_type> methods;
		std::vector<posetcons_type> strats;
		bool silent = false;
		size_t jobs = 1;
	};

	static int read_options(cli_options& opt, int argc, char** argv)
	{
		std::string methods, strats;

		boost::program_options::options_description o_general("Options");
		o_general.add_options()
				("help,h", "display this message")
				("silent,s", "do not print progress")
				("corpii,c", boost::program_options::value(&opt.corpii), "select which corpii to sample or generate, possibly comma separated (default: all)")
				("methods,m", boost::program_options::value(&methods), "select which methods to use, possibly comma separated (default: all)")
				("strats,r", boost::program_options::value(&strats), "select which poset consistency strategies to use, possibly comma separated (default: all)")
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
					<< "  inspect        inspect all objects" << std::endl
					<< "  measure        run all scheduled tests and store the results" << std::endl
					<< "  report         report on all results" << std::endl
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

		if(!vm.count("corpii"))
		{
			opt.corpii = "all";
		}

		if(!vm.count("methods") || methods == "all")
			opt.methods = {ml_type::knn, ml_type::knn_adaptive, ml_type::naive_bayes, ml_type::omniscient};
		else
		{
			std::vector<std::string> methods_arr;
			boost::algorithm::split(methods_arr, methods, boost::algorithm::is_any_of(","));

			for(auto m : methods_arr)
				opt.methods.emplace_back(to_ml_type(m));
		}

		if(!vm.count("strats") || strats == "all")
		{
			opt.strats = {posetcons_type::canonical, posetcons_type::optimistic, posetcons_type::pessimistic};
		}
		else
		{
			std::vector<std::string> strats_arr;
			boost::algorithm::split(strats_arr, strats, boost::algorithm::is_any_of(","));

			for(auto s : strats_arr)
				opt.strats.emplace_back(to_posetcons_type(s));
		}

		return EXIT_SUCCESS;
	}

public:
	cli() = delete;

	static int exec(int argc, char** argv)
	{
		cli_options opt;

		int rv = read_options(opt, argc, argv);
		if(rv != EXIT_SUCCESS)
			return rv;

		std::vector<std::string> corpii;
		if(opt.corpii == "all")
			corpii = {"Coq", "CoRN", "CoRN-legacy", "ch2o", "mathcomp", "MathClasses"}; // TODO automate
		else
			boost::algorithm::split(corpii, opt.corpii, boost::algorithm::is_any_of(","));

		if(opt.action == "inspect")
		{
			for(auto&& corpus : corpii)
				for(auto&& method : opt.methods)
				{
					auto const d(storage::read_dataset(corpus));
					inspector::iterate_all(method, d);
				}
		}
		else if(opt.action == "measure")
		{
			for(auto&& corpus : corpii)
				for(auto&& strat : opt.strats)
					for(auto&& method : opt.methods)
					{
						tester::exec(corpus, strat, method, opt.jobs, opt.silent);
					}
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
		else if(opt.action == "report")
		{
			storage::read_result([&corpii](cv_result_t const& result) {
				if(!std::any_of(corpii.begin(), corpii.end(), [&result](std::string const& y) {
					return y == result.corpus;
				}))
					return;

				std::cout << result << std::endl;
			});
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
