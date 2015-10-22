#pragma once

#include <functional>

namespace std
{

template<bool B, class T = void>
using enable_if_t = typename enable_if<B,T>::type;

}

namespace roerei
{

template<typename F, typename FARG>
using is_function = typename std::is_constructible<std::function<FARG>, F>;

}
