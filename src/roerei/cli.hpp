#pragma once


#include <roerei/generator.hpp>
#include <roerei/inspector.hpp>
#include <roerei/tester.hpp>

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
		std::string strats;
		bool silent = false;
		size_t jobs = 1;
	};

	static int read_options(cli_options& opt, int argc, char** argv)
	{
		boost::program_options::options_description o_general("Options");
		o_general.add_options()
				("help,h", "display this message")
				("silent,s", "do not print progress")
				("corpii,c", boost::program_options::value(&opt.corpii), "select which corpii to sample or generate, possibly comma separated (default: all)")
				("strats,r", boost::program_options::value(&opt.strats), "select which poset consistency strategies to use, possibly comma separated (default: all)")
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

		if(!vm.count("strats"))
		{
			opt.strats = "all";
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

		std::vector<std::string> corpii;
		if(opt.corpii == "all")
			corpii = {"Coq", "CoRN", "ch2o", "mathcomp", "MathClasses"}; // TODO automate
		else
			boost::algorithm::split(corpii, opt.corpii, boost::algorithm::is_any_of(","));

		std::vector<std::string> strats;
		if(opt.strats == "all")
			strats = {"canonical", "optimistic", "pessimistic"};
		else
			boost::algorithm::split(strats, opt.strats, boost::algorithm::is_any_of(","));

		if(opt.action == "inspect")
		{
			for(auto&& corpus : corpii)
			{
				auto const d(storage::read_dataset(corpus));
				inspector::iterate_all(d);
			}
		}
		else if(opt.action == "measure")
		{
			for(auto&& corpus : corpii)
				for(auto&& strat : strats)
					tester::exec(corpus, strat, opt.jobs, opt.silent);
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
		else if(opt.action == "test")
		{
			for(auto&& corpus : corpii)
			{
				auto const d(storage::read_dataset(corpus));
				auto const d_new(posetcons_canonical::consistentize(d));

				// Does consistensizing change order?
				if(d.objects == d_new.objects)
					std::cerr << "<Notice> Nothing changed" << std::endl;

				// Is a consistent dataset truly consistent?
				auto dependants_trans(dependencies::create_obj_dependants(d));
				dependants_trans.transitive();

				posetcons_canonical pc;

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
