/**
 * \file
 * \brief A vector with max number of elements specified in compile time
 *
 * FIXME: this is sort of useless, if I add short-length-optimized vector.
 * TODO: Maybe add proper iterators? 
 *       Altho there is a question -- why do that?
 */
#pragma once
#include <cstddef>
#include <cassert>
#include <initializer_list>
#include <utility>

namespace misc {

template<typename T, size_t MaxLen>
class ShortVec {
    union { T m_data[MaxLen]; }; // a trick to not construct objects there
    size_t m_size = 0;
public:

    ShortVec() {};

    ShortVec(const std::initializer_list<T> &l) {
        assert(m_size <= MaxLen);
        m_size = l.size();
        size_t pos = 0;
        for (const auto &v : l)
            new (&m_data[pos++]) T(v);
    }

    ShortVec(const ShortVec &ref) {
    	// same as templated one, but in this form
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(ref[i]);
    }

    template<size_t OtherLen>
    ShortVec(const ShortVec<T, OtherLen> &ref) {
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(ref[i]);
    }

    ShortVec &operator=(const ShortVec &ref) {
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(ref[i]);
        return *this;
    }

    template<size_t OtherLen>
    ShortVec &operator=(const ShortVec<T, OtherLen> &ref) {
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(ref[i]);
        return *this;
    }

    ShortVec(ShortVec &&ref) {
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(std::move(ref[i]));
    }

    template<size_t OtherLen>
    ShortVec(ShortVec<T, OtherLen> &&ref) {
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(std::move(ref[i]));
    }

    ShortVec &operator=(ShortVec &&ref) {
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(std::move(ref[i]));
        return *this;
    }

    template<size_t OtherLen>
    ShortVec &operator=(ShortVec<T, OtherLen> &&ref) {
        assert(ref.size() <= MaxLen);
        m_size = ref.size();
        for (size_t i = 0; i < ref.size(); ++i)
            new (&m_data[i]) T(std::move(ref[i]));
        return *this;
    }

    const T *data()               const { return m_data; }
    T       *data()                     { return m_data; }

    size_t   size()               const { return m_size; }
    bool     isEmpty()            const { return m_size == 0; }
    bool     isFull()             const { return m_size == MaxLen; }

    const T &operator[](size_t i) const { return m_data[i]; }
          T &operator[](size_t i)       { return m_data[i]; }

    const T &last()  const { assert(!isEmpty()); return m_data[m_size-1]; } 
          T &last()        { assert(!isEmpty()); return m_data[m_size-1]; }

    const T &first() const { assert(!isEmpty()); return m_data[0]; }
          T &first()       { assert(!isEmpty()); return m_data[0]; }

    const T *begin() const { return &m_data[0]; }
          T *begin()       { return &m_data[0]; }
    const T *end()   const { return &m_data[m_size]; }
          T *end()         { return &m_data[m_size]; }

    void resize(size_t newSize) {
        assert(newSize <= MaxLen);
        while (newSize > m_size)
            new (&m_data[m_size++]) T();
        while (newSize < m_size)
            (m_data[--m_size]).~T();
        m_size = newSize;
    }

    void push(const T &v) {
        assert(!isFull());
        new (&m_data[m_size]) T(v);
        m_size++;
    }

    void push(T &&v) {
        assert(!isFull());
        new (&m_data[m_size]) T(std::move(v));
        m_size++;
    }

    T pop() {
        assert(!isEmpty());
        T item = std::move(m_data[m_size-1]);
        (m_data[m_size-1]).~T();
        m_size--;
        return item;
    }

    ~ShortVec() {
        for (size_t i = 0; i < size(); ++i)
            (m_data[i]).~T();
        m_size = 0;
    }
};

};
