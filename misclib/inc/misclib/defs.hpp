/**
 * \file
 * \brief Some typedefs for long-named C++ types, and asserts
 */
#pragma once

#include <string_view>
#include <memory>

namespace misc {

using View = std::string_view;

template<typename T>
using UPtr = std::unique_ptr<T>;

};


