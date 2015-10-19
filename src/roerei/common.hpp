#pragma once

namespace roerei
{

template<typename F, typename FARG>
using is_function = typename std::is_constructible<std::function<FARG>, F>;

}
