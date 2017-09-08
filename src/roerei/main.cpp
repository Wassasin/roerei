#include <roerei/cpp14_fix.hpp>
#include <roerei/cli.hpp>
#include <roerei/util/performance.hpp>
#include <roerei/generic/multitask.hpp>

size_t roerei::multitask::jobset_t::jobset_count = 0;

register_performance

int main(int argc, char** argv)
{
	return roerei::cli::exec(argc, argv);
}
