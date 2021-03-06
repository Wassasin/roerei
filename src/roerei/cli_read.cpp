#include <roerei/cpp14_fix.hpp>
#include <roerei/cli.hpp>

#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <iostream>

namespace roerei
{

int cli::read_options(cli_options& opt, int argc, char** argv)
{
	std::string corpii, methods, strats, filter;

	boost::program_options::options_description o_general("Options");
	o_general.add_options()
			("help,h", "display this message")
			("silent,s", "do not print progress")
			("no-prior,P", "do not use prior datasets, if applicable")
      ("no-cv,C", "do not use crossvalidation, if applicable")
			("corpii,c", boost::program_options::value(&corpii), "select which corpii to sample or generate, possibly comma separated (default: all)")
			("methods,m", boost::program_options::value(&methods), "select which methods to use, possibly comma separated (default: all)")
			("strats,r", boost::program_options::value(&strats), "select which poset consistency strategies to use, possibly comma separated (default: all)")
			("jobs,j", boost::program_options::value(&opt.jobs), "number of concurrent jobs (default: 1)")
			("filter,f", boost::program_options::value(&filter), "show only objects which include the filter string");

	boost::program_options::variables_map vm;
	boost::program_options::positional_options_description pos;

	pos.add("action", 1);
	pos.add("args", 2); // Max 2 for diff

	boost::program_options::options_description options("Allowed options");
	options.add(o_general);
	options.add_options()
			("action", boost::program_options::value(&opt.action))
			("args", boost::program_options::value(&opt.args));

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
				<< "  generate                 load repo.msgpack, convert and write to dataset.msgpack" << std::endl
				<< "  inspect                  inspect all objects" << std::endl
				<< "  measure                  run all scheduled tests and store the results" << std::endl
				<< "  report [results]         report on all results [in file 'results']" << std::endl
				<< "  diff <c1> <c2>           show the difference between two corpii" << std::endl
				<< "  export [results] <dest>  dump all results [from file 'results-path'] into predefined format for thesis in directory <path>" << std::endl
				<< "  upgrade <src>						 upgrade non-prior results dataset to newest version" << std::endl
				<< "  legacy-export            export dataset in the legacy format" << std::endl
				<< "  legacy-import            import dataset in the legacy format" << std::endl
				<< std::endl
				<< o_general;

		return EXIT_FAILURE;
	}

	if (vm.count("silent")) {
		opt.silent = true;
	}

	if (vm.count("no-prior")) {
		opt.prior = false;
	}

  if (vm.count("no-cv")) {
    opt.cv = false;
  }

	if(!vm.count("action"))
	{
		std::cerr << "Please specify an action, see --help." << std::endl;
		return EXIT_FAILURE;
	}

	if(!vm.count("corpii") || corpii == "all")
	{
		// TODO automate
		for(std::string corpus : {"Coq", "CoRN", "ch2o", "mathcomp", "MathClasses"}) {
			opt.corpii.emplace_back(corpus+".frequency");
			opt.corpii.emplace_back(corpus+".depth");
			opt.corpii.emplace_back(corpus+".flat");
		}
	}
	else
		boost::algorithm::split(opt.corpii, corpii, boost::algorithm::is_any_of(","));

	if(!vm.count("methods") || methods == "all")
		opt.methods = {ml_type::knn, ml_type::knn_adaptive, ml_type::naive_bayes, ml_type::omniscient, ml_type::ensemble, ml_type::adarank};
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

	if(vm.count("filter"))
	{
		opt.filter = filter;
	}

	return EXIT_SUCCESS;
}

}
