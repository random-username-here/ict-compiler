#include "scl/ast/type.hpp"
#include "ict/ir.hpp"
#include "misclib/dump_stream.hpp"
#include <ostream>
using namespace misc::color;

namespace scl {

bool Type::isInteger() const {
    auto p = dynamic_cast<const PrimitiveType*>(this);
    return p && p->kind() == PrimitiveType::INT64;
}

bool Type::isVoid() const {
    auto p = dynamic_cast<const PrimitiveType*>(this);
    return p && p->kind() == PrimitiveType::VOID;
}

bool Type::isPointer() const {
    auto p = dynamic_cast<const PointerType*>(this);
    return p != nullptr;
}
bool Type::isFunction() const {
    auto f = dynamic_cast<const FunctionType*>(this);
    return f != nullptr;
}
bool Type::isFunctionPtr() const {
    auto p = dynamic_cast<const PointerType*>(this);
    return p != nullptr && p->to()->isFunction();
}

bool Type::isPack() const {
    auto p = dynamic_cast<const PackType*>(this);
    return p != nullptr;
}

size_t PrimitiveType::byteSize() const {
#define X(id, sn, sz, ic) if (m_kind == id) return sz;
SCL_FOR_EACH_TYPE(X)
#undef X
    return 0;
}

size_t PrimitiveType::alignment() const {
#define X(id, sn, sz, ic) if (m_kind == id) return sz ? sz : 1;
SCL_FOR_EACH_TYPE(X)
#undef X
    return 1;
}

UPtr<ict::Type> PrimitiveType::toICT() const {
#define X(id, sn, sz, ic) if (m_kind == id) return ic;
SCL_FOR_EACH_TYPE(X)
#undef X
    return nullptr;
}

void PrimitiveType::dump(std::ostream &os) const {
    os << CYAN;
#define X(id, sn, sz, ic) if (m_kind == id) os << sn;
SCL_FOR_EACH_TYPE(X)
#undef X
    os << RST;
}
    
std::optional<PrimitiveType::Kind> PrimitiveType::kindFromString(misc::View name) {
#define X(id, sn, sz, ic) if (name == sn) return id;
SCL_FOR_EACH_TYPE(X)
#undef X
    return std::nullopt;
}

bool PrimitiveType::isSameAs(const Type *other) const {
    auto p = dynamic_cast<const PrimitiveType*>(other);
    if (!p) return false;
    return p->kind() == kind();
}

void PointerType::dump(std::ostream &os) const {
    os << BLUE << "*" << RST << *to();
}
 
bool PointerType::isSameAs(const Type *other) const {
    auto p = dynamic_cast<const PointerType*>(other);
    if (!p) return false;
    return p->to()->isSameAs(to().get());
}   

void FunctionType::dump(std::ostream &os) const {
    os << RED << "func " << DGRAY << "(" << RST;
    for (size_t i = 0; i < args().size(); ++i) {
        if (i != 0) os << DGRAY << ", " << RST;
        os << *args()[i];
    }
    os << DGRAY << ") " << RST << *returnType();
}

bool FunctionType::isSameAs(const Type *other) const {
    auto f = dynamic_cast<const FunctionType*>(other);
    if (!f) return false;
    if (!f->returnType()->isSameAs(returnType().get())) return false;
    if (f->args().size() != args().size()) return false;
    for (size_t i = 0; i < args().size(); ++i)
        if (!f->args()[i]->isSameAs(args()[i])) return false;
    return true;
}

void PackType::dump(std::ostream &os) const {
    os << BLUE << "(arg pack)" << RST;
}

bool PackType::isSameAs(const Type *o) const {
    return dynamic_cast<const PackType*>(o) != nullptr;
}

};
