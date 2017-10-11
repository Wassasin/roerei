#pragma once

#include <roerei/cli_options.hpp>

namespace roerei
{

class cli
{
private:
	static int read_options(cli_options& opt, int argc, char** argv);
	static void exec_inspect(cli_options& opt);
	static void exec_measure(cli_options& opt);
	static void exec_generate(cli_options& opt);
	static void exec_report(cli_options& opt);
	static void exec_export(cli_options& opt);
	static void exec_diff(cli_options& opt);
	static void exec_upgrade(cli_options& opt);

public:
	cli() = delete;

	static int exec(int argc, char** argv);
};

}
