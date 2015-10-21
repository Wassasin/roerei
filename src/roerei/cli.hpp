#pragma once

#include <roerei/generator.hpp>
#include <roerei/partition.hpp>
#include <roerei/distance.hpp>
#include <roerei/knn.hpp>

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
					<< "  generate       load repo.msgpack, convert and write to dataset.msgpack" << std::endl
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
			auto const d(storage::read_dataset());
			knn c(5, d.feature_matrix);

			auto print_f([&](dataset_t::matrix_t::const_row_proxy_t const& row) {
				for(auto const& kvp : row)
					std::cout << " " << kvp.second << '*' << kvp.first;
				std::cout << std::endl;
			});

			for(size_t i = 0; i < d.feature_matrix.m; ++i)
			{
				std::cout << i << ":";
				print_f(d.feature_matrix[i]);
				std::cout << "----" << std::endl;

				for(auto const& kvp : c.predict(d.feature_matrix[i]))
				{
					std::cout << kvp.first << ": " << kvp.second << " <=";
					print_f(d.feature_matrix[kvp.first]);
				}
				std::cout << std::endl;
			}

		}
		else if(opt.action == "generate")
		{
			auto const d(generator::construct_from_repo());
			storage::write_dataset(d);
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
