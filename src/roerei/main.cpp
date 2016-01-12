#include <initializer_list> // force libstdc++ to include its config
#undef _GLIBCXX_HAVE_GETS // correct broken config

#include <roerei/cli.hpp>

register_performance

int main(int argc, char** argv)
{
	return roerei::cli::exec(argc, argv);
}
