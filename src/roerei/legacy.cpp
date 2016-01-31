#include <initializer_list> // force libstdc++ to include its config
#undef _GLIBCXX_HAVE_GETS // correct broken config

#include <roerei/util/performance.hpp>
#include <roerei/legacy.hpp>

register_performance

int main(int /*argc*/, char** /*argv*/)
{
	roerei::legacy::exec();
	return EXIT_SUCCESS;
}
