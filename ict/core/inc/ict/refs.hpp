#pragma once
#include "misclib/array_view.hpp"
#include <algorithm>
#include <cassert>
#include <vector>
namespace ict {

// TODO: llvm style uses
// also good for destroying ui?

template<typename Self, typename To>
class Ref {
    template<typename _>
    friend class Refable;

    To *m_to;
public:
    Ref() :m_to(nullptr) {}
    Ref(To *to) :m_to(to) {
        if (to)
            to->m_ref(static_cast<Self*>(this));
    }
    Ref(const Ref &o) :m_to(o.m_to) {
        if (m_to)
            m_to->m_ref(static_cast<Self*>(this));
    }
    Ref &operator=(const Ref &o) {
        m_to = o.m_to;
        if (m_to)
            m_to->m_ref(static_cast<Self*>(this));
    }

    ~Ref() {
        if (m_to)
            m_to->m_unref(static_cast<Self*>(this));
    }

    To *ptr() { return m_to; }
    const To *ptr() const { return m_to; }

};

template<typename By>
class Refable {
    template<typename Self, typename To>
    friend class Ref;

    std::vector<By*> m_refs;

    void m_ref(By *by) {
        m_refs.push_back(by);
    }
    void m_unref(By *by) {
        auto pos = std::find(m_refs.begin(), m_refs.end(), by);
        assert(pos != m_refs.end());
        m_refs.erase(pos);
    }

public:

    Refable() = default;

    Refable(const Refable &o) = delete;
    Refable &operator=(const Refable &o) = delete;

    // TODO: allow moving?
    Refable(Refable &&o) = delete;
    Refable &operator=(Refable &&o) = delete;


    misc::ArrayView<By* const> refs() const { return m_refs; }

    ~Refable() {
        for (auto i : m_refs)
            i->m_to = nullptr;
        m_refs.clear();
    }
};

}
