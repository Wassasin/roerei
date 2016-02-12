#include <roerei/cli.hpp>

#include <roerei/generator.hpp>
#include <roerei/storage.hpp>
#include <roerei/inspector.hpp>
#include <roerei/tester.hpp>
#include <roerei/legacy.hpp>

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
	std::map<std::string, dataset_t> map(generator::construct_from_repo());
	for(auto const& kvp : map)
	{
		storage::write_dataset(kvp.first, kvp.second);
		std::cerr << "Written " << kvp.first << std::endl;
	}
}

void cli::exec_inspect(cli_options& opt)
{
	for(auto&& corpus : opt.corpii)
		for(auto&& method : opt.methods)
		{
			auto const d(storage::read_dataset(corpus));
			inspector::iterate_all(method, d);
		}
}

void cli::exec_report(cli_options& opt)
{
	storage::read_result([&opt](cv_result_t const& result) {
		if(!std::any_of(opt.corpii.begin(), opt.corpii.end(), [&result](std::string const& y) {
			return y == result.corpus;
		}))
			return;

		std::cout << result << std::endl;
	});
}

void cli::exec_measure(cli_options& opt)
{
	for(auto&& corpus : opt.corpii)
		for(auto&& strat : opt.strats)
			for(auto&& method : opt.methods)
			{
				tester::exec(corpus, strat, method, opt.jobs, opt.silent);
			}
}

}
