#include <roerei/cpp14_fix.hpp>
#include <roerei/cli.hpp>
#include <roerei/util/performance.hpp>

register_performance

int main(int argc, char** argv)
{
	return roerei::cli::exec(argc, argv);
}
