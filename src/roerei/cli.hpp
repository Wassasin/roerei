#pragma once

#include <roerei/cv.hpp>
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
					<< "  cv             test performance using cross-validation" << std::endl
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
			knn<decltype(d.feature_matrix)> c(5, d.feature_matrix);

			auto print_f([&](dataset_t::matrix_t::const_row_proxy_t const& row) {
				std::cout << "[";
				for(auto const& kvp : row)
					std::cout << " " << kvp.second << '*' << kvp.first;
				std::cout << " ]";
			});

			for(size_t i = 0; i < d.feature_matrix.m; ++i)
			{
				std::cout << i << " " << d.objects[i] << " ";
				print_f(d.feature_matrix[i]);
				std::cout << std::endl;

				std::cout << "-- Required" << std::endl;
				{
					bool empty = true;
					for(auto const& kvp : d.dependency_matrix[i])
					{
						empty = false;
						std::cout << kvp.second << "*" << kvp.first << " " << d.dependencies[kvp.first] << std::endl;
					}
					if(empty)
						std::cout << "Empty set" << std::endl;
				}

				std::cout << "-- Similar" << std::endl;

				std::map<size_t, float> suggestions; // <id, weighted freq>

				auto predictions(c.predict(d.feature_matrix[i]));
				std::reverse(predictions.begin(), predictions.end());
				for(auto const& kvp : predictions)
				{
					if(kvp.first == i)
						continue;

					std::cout << kvp.second << " <= " << kvp.first << " " << d.objects[kvp.first] << " ";
					print_f(d.feature_matrix[kvp.first]);
					std::cout << std::endl;

					float weight = 1.0f / (kvp.second + 1.0f); // "Similarity", higher is more similar

					for(auto dep_kvp : d.dependency_matrix[kvp.first])
						suggestions[dep_kvp.first] += ((float)dep_kvp.second) * weight;
				}

				std::vector<std::pair<size_t, float>> suggestions_sorted;
				for(auto const& kvp : suggestions)
					suggestions_sorted.emplace_back(kvp);

				std::sort(suggestions_sorted.begin(), suggestions_sorted.end(), [&](std::pair<size_t, float> const& x, std::pair<size_t, float> const& y) {
					return x.second > y.second;
				});

				std::set<size_t> required_deps, suggested_deps, found_deps, missing_deps;
				for(auto const& kvp : d.dependency_matrix[i])
					required_deps.insert(kvp.first);

				for(size_t j = 0; j < suggestions_sorted.size() && j < 100; ++j)
					suggested_deps.insert(suggestions_sorted[j].first);

				std::set_intersection(
					required_deps.begin(), required_deps.end(),
					suggested_deps.begin(), suggested_deps.end(),
					std::inserter(found_deps, found_deps.begin())
				);

				std::set_difference(
					required_deps.begin(), required_deps.end(),
					found_deps.begin(), found_deps.end(),
					std::inserter(missing_deps, missing_deps.begin())
				);

				std::cout << "-- Suggestions" << std::endl;
				for(size_t j = 0; j < suggestions_sorted.size(); j++)
				{
					auto const& kvp = suggestions_sorted[j];
					if(required_deps.find(kvp.first) != required_deps.end())
						std::cout << "! ";

					std::cout << kvp.second << " <= " << kvp.first << " " << d.dependencies[kvp.first] << std::endl;
				}

				std::cout << "-- Missing" << std::endl;
				{
					bool empty = true;
					for(size_t j : missing_deps)
					{
						empty = false;
						std::cout << d.dependency_matrix[i][j] << "*" << j << " " << d.dependencies[j] << std::endl;
					}
					if(empty)
						std::cout << "Empty set" << std::endl;
				}

				std::cout << "-- Metrics" << std::endl;
				float c_required = required_deps.size();
				float c_suggested = suggested_deps.size();
				float c_found = found_deps.size();

				float oocover = c_found/c_required;
				float ooprecision = c_found/c_suggested;

				if(required_deps.empty())
				{
					oocover = 1.0f;
					ooprecision = 1.0f;
				}

				std::cout << "100Cover: " << oocover << std::endl;
				std::cout << "100Precision: " << ooprecision << std::endl;

				std::cout << std::endl;
			}
		}
		else if(opt.action == "cv")
		{
			auto const d(storage::read_dataset());

			cv::exec(d, 10, 3);
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
