#pragma once
#include "misclib/defs.hpp"

namespace scl {

class Decl {
    std::string m_name;
public:
    Decl(misc::View n) :m_name(n) {}
    misc::View name() const { return m_name; }
    virtual void dump(std::ostream &os) const = 0;
};

};
