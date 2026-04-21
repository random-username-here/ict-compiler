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

/**
 * Declare `::create()` function.
 */
#define MISC_CREATEFUNC(cls)\
    template<typename ...Args>\
    static misc::UPtr<cls> create(Args ...args) {\
        return std::make_unique<cls>(std::forward<decltype(args)>(args)...);\
    }

};


