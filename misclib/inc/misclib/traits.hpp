#pragma once
#include <type_traits>

namespace misc {

namespace impl {
    template<typename T, template<typename...> typename Cls>
    struct IsExactSpecializationT : std::false_type {};

    template<template<typename...> typename Cls, typename... Args>
    struct IsExactSpecializationT<Cls<Args...>, Cls> : std::true_type {};
};

/// Check if class is a clean specialization.
/// For example, `std::list<int>` is a specialization of `std::list`,
/// but `const std::list<int>` is not
template<typename T, template<typename...> typename Cls>
inline constexpr bool IsExactSpecialization
    = impl::IsExactSpecializationT<T, Cls>::value;

/// Check if class is a specialization, with no regard to type
/// being a reference/constant/volatile.
template<typename T, template<typename...> typename Cls>
inline constexpr bool IsSpecialization 
    = IsExactSpecialization<std::remove_reference_t<std::remove_cv_t<T>>, Cls>;

};
