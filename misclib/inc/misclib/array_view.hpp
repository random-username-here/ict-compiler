#pragma once
#include <cstddef>
#include <initializer_list>
#include <type_traits>
// why do I keep writting standard library instead of doing something usefull??
// TODO: tests

namespace misc {


template<typename T>
class ArrayView_ {
    T m_data;
    size_t m_size;

public:

    using ElementType = std::remove_pointer_t<T>;

    ArrayView_(const std::initializer_list<ElementType> &v) {
        m_data = v.begin();
        m_size = v.end() - v.begin();
    }

    template<size_t Size, typename T_ = T, std::enable_if_t<std::is_const_v<T_>> = true>
    constexpr ArrayView_(const ElementType arr[Size]) {
        static_assert(std::is_const_v<T>, "Value type must be const for constructing ArrayView from static arrays");
        m_data = arr; m_size = Size;
    }

    template<typename Cont, typename T_ = T, std::enable_if_t<std::is_const_v<T_>> = true>
    ArrayView_(const Cont &arr) {
        m_data = arr.data(); m_size = arr.size();
    }

    template<typename Cont>
    ArrayView_(Cont &arr) {
        m_data = arr.data(); m_size = arr.size();
    }

    // ArrayView<T> -> ArrayView<const T>
    template<typename T_ = T, std::enable_if_t<std::is_const_v<T_>> = true>
    ArrayView_(const ArrayView_<std::remove_const_t<T_>> &arr)
        :m_data(arr.m_data), m_size(arr.m_size) {}

    ArrayView_(T* data, size_t size) {
        m_data = data; m_size = size;
    }

    T *data() { return m_data; }
    const T *data() const { return m_data; }
    size_t size() const { return m_size; }
    const ElementType &operator[](size_t idx) const { return m_data[idx]; }
    ElementType &operator[](size_t idx) { return m_data[idx]; }

    T begin() { return m_data; }
    T end() { return m_data + m_size; }
    const T begin() const { return m_data; }
    const T end() const { return m_data + m_size; }
};


template<typename Element>
ArrayView_(const std::initializer_list<Element> &) -> ArrayView_<const Element*>;

template<typename T>
using ArrayView = ArrayView_<T*>;

template<typename T>
using ConstArrayView = ArrayView_<const T*>;

};
