#pragma once

/* Solves C++14-related error for distro's with broken stdlib
 * /usr/lib/gcc/x86_64-linux-gnu/4.8/../../../../include/c++/4.8/cstdio:120:11: error: no member named 'gets' in the global namespace
 * using ::gets;
 *       ~~^
 */

#include <initializer_list>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#undef _GLIBCXX_HAVE_GETS
#pragma clang diagnostic pop
