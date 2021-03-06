#include <roerei/cpp14_fix.hpp>
#include <roerei/cli.hpp>

#include <roerei/generator.hpp>
#include <roerei/sampler.hpp>
#include <roerei/storage.hpp>
#include <roerei/inspector.hpp>
#include <roerei/tester.hpp>
#include <roerei/legacy.hpp>
#include <roerei/diff.hpp>
#include <roerei/exporter.hpp>

namespace roerei
{

int cli::exec(int argc, char** argv)
{
	cli_options opt;

	int rv = read_options(opt, argc, argv);
	if(rv != EXIT_SUCCESS)
		return rv;

	if(opt.action == "inspect")
		exec_inspect(opt);
	else if(opt.action == "measure")
		exec_measure(opt);
	else if(opt.action == "generate")
		exec_generate(opt);
	else if(opt.action == "report")
		exec_report(opt);
	else if(opt.action == "export")
		exec_export(opt);
	else if(opt.action == "diff")
		exec_diff(opt);
	else if(opt.action == "upgrade")
		exec_upgrade(opt);
	else if(opt.action == "legacy-export")
	{
		auto const d(storage::read_dataset("CoRN-legacy"));
		storage::write_legacy_dataset("/home/wgeraedts/tmp/new", d);
	}
	else if(opt.action == "legacy-import")
	{
		auto const d(legacy::read("/home/wgeraedts/tmp", "CoRN-legacy"));
		storage::write_dataset("CoRN-legacy", d);
	}
	else
	{
		std::cerr << "Unknown action '" << opt.action << "', see --help." << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void cli::exec_generate(cli_options& /*opt*/)
{
	auto generate = [](generator::variant_e variant, std::string postfix) {
		std::map<std::string, dataset_t> map(generator::construct_from_repo(variant));
		for(auto const& kvp : map)
		{
			auto name = kvp.first+"."+postfix;
			storage::write_dataset(name, kvp.second);
			std::cerr << "Written " << name << std::endl;

            auto sample_name = kvp.first+".sample."+postfix;
            dataset_t d_sample(sampler::sample(kvp.second));
            storage::write_dataset(sample_name, d_sample);
            std::cerr << "Written " << sample_name << std::endl;
		}
	};

	generate(generator::variant_e::frequency, "frequency");
	generate(generator::variant_e::depth, "depth");
	generate(generator::variant_e::flat, "flat");
}

void cli::exec_inspect(cli_options& opt)
{
	for(auto&& corpus : opt.corpii)
		for(auto&& method : opt.methods)
		{
			auto const d(storage::read_dataset(corpus));
			inspector::iterate_all(method, d, opt.filter);
		}
}

void cli::exec_report(cli_options& opt)
{
	std::string path = "./data/results.msgpack";

	if(opt.args.size() > 1)
		throw std::runtime_error("Too many arguments for report");
	else if(opt.args.size() == 1)
		path = opt.args[0];

	storage::read_result([&opt](cv_result_t const& result) {
		if(!std::any_of(opt.corpii.begin(), opt.corpii.end(), [&result](std::string const& y) {
			return y == result.corpus;
		}))
			return;

		std::cout << result << std::endl;
	}, path);
}

void cli::exec_diff(cli_options& opt)
{
	if(opt.args.size() != 2)
		throw std::runtime_error("Incorrect number of arguments for diff");

	diff::exec(opt.args[0], opt.args[1]);
}

void cli::exec_measure(cli_options& opt)
{
	multitask m;

	for(auto&& corpus : opt.corpii) {
		for(auto&& strat : opt.strats) {
			for(auto&& method : opt.methods) {
        tester::order(m, corpus, strat, method, opt.prior, opt.silent, opt.cv);
			}
		}
	}

	m.run(opt.jobs, true); // Blocking
}

void cli::exec_export(cli_options& opt)
{
	std::string source_path = "./data/results.msgpack";
	std::string output_path;

	if(opt.args.size() > 2) {
		throw std::runtime_error("Too many arguments for report");
	} else if(opt.args.size() == 2) {
		source_path = opt.args[0];
		output_path = opt.args[1];
	} else if(opt.args.size() == 1) {
		output_path = opt.args[0];
	}
	
	exporter::exec(source_path, output_path);
}

void cli::exec_upgrade(cli_options& opt)
{
	if (opt.args.size() != 1) {
		throw std::runtime_error("Incorrect number of arguments for upgrade");
	}

	std::vector<cv_result_t> results;

	storage::read_v1_result([&opt, &results](cv_result_v1_t const& r) {
		results.emplace_back((cv_result_t){
			r.corpus,
			opt.prior,
			r.strat,
			r.ml,
			r.knn_params,
			r.nb_params,
			r.adarank_params,
			r.n, r.k,
			r.metrics
		});
	}, opt.args[0]);

	storage::read_result([&opt, &results](cv_result_t const& r) {
		for (auto it = results.begin(); it != results.end(); ++it) {
			auto const& new_r = *it;
			if (result_same_base(r, new_r)) {
				if (r == new_r) {
					std::cout << "Duplicate " << new_r << std::endl;
				} else {
					std::cout << "WARNING: Incompatible " << new_r << std::endl;
				}

				results.erase(it);
				break;
			}
		}
	});

	if (results.empty()) {
			std::cout << "Note: no entries written" << std::endl;
	} else {
		for (cv_result_t const& result : results) {
			std::cout << "Wrote " << result << std::endl;
			storage::write_result(result);
		}
	}
}

}
