#pragma once

#include <roerei/storage.hpp>

#include <set>
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
	};

	static int read_options(cli_options& opt, int argc, char** argv)
	{
		boost::program_options::options_description o_general("Options");
		o_general.add_options()
				("help,h", "display this message");

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
					<< "  test           placeholder action" << std::endl
					<< std::endl
					<< o_general;

			return EXIT_FAILURE;
		}

		if(!vm.count("action"))
		{
			std::cerr << "Please specify an action, see --help." << std::endl;
			return EXIT_FAILURE;
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
			storage s;

			std::set<uri_t> objects, dependencies;
			s.read_summaries([&](summary_t&& s) {
				for(auto&& t : s.type_uris)
				{
					objects.emplace(t.uri); // Copy
					dependencies.emplace(std::move(t.uri));
				}

				if(s.body_uris)
					for(auto&& b : *s.body_uris)
					{
						objects.emplace(std::move(b.uri));
					}
			});

			std::set<uri_t> theorems;
			std::set_difference(objects.begin(), objects.end(), dependencies.begin(), dependencies.end(), std::inserter(theorems, theorems.begin()));

			std::cout << "Objects: " << objects.size() << std::endl;
			std::cout << "Dependencies: " << dependencies.size() << std::endl;
			std::cout << "Theorems: " << theorems.size() << std::endl;
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
