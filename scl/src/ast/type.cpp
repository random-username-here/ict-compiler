#include "scl/ast/type.hpp"
#include "ict/ir.hpp"
#include "misclib/dump_stream.hpp"
#include <ostream>
using namespace misc::color;

namespace scl {

void Type::dumpQualifiers(std::ostream &os) const {
    if (isConst())
        os << YELLOW << "const " << RST;
}

bool Type::isInteger() const {
    auto p = dynamic_cast<const PrimitiveType*>(unwrapNamed());
    if (!p) return false;
    switch (p->kind()) {
        case PrimitiveType::INT8: case PrimitiveType::INT16:
        case PrimitiveType::INT32: case PrimitiveType::INT64:
            return true;
        default:
            return false;
    }
}

bool Type::isVoid() const {
    auto p = dynamic_cast<const PrimitiveType*>(unwrapNamed());
    return p && p->kind() == PrimitiveType::VOID;
}

bool Type::isBool() const {
    auto p = dynamic_cast<const PrimitiveType*>(unwrapNamed());
    return p && p->kind() == PrimitiveType::BOOL;
}

bool Type::isPointer() const {
    auto p = dynamic_cast<const PointerType*>(unwrapNamed());
    return p != nullptr;
}
bool Type::isFunction() const {
    auto f = dynamic_cast<const FunctionType*>(unwrapNamed());
    return f != nullptr;
}
bool Type::isFunctionPtr() const {
    auto p = dynamic_cast<const PointerType*>(unwrapNamed());
    return p != nullptr && p->to()->isFunction();
}

bool Type::isPack() const {
    auto p = dynamic_cast<const PackType*>(unwrapNamed());
    return p != nullptr;
}

bool Type::isType() const {
    auto p = dynamic_cast<const TypeType*>(unwrapNamed());
    return p != nullptr;
}

bool Type::isNamed() const {
    auto p = dynamic_cast<const NamedType*>(this);
    return p != nullptr;
}

const Type *Type::unwrapNamed() const {
    auto t = this;
    while (1) {
        auto n = dynamic_cast<const NamedType*>(t);
        if (!n) break;
        if (!n->isResolved()) break;
        t = n->resolved().get();
    }
    return t;
}

bool Type::areSame(const Type *a, const Type *b) {
    return a->unwrapNamed()->isSameAs(b->unwrapNamed());
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
    dumpQualifiers(os);
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
    dumpQualifiers(os);
    os << BLUE << "*" << RST << *to();
}
 
bool PointerType::isSameAs(const Type *other) const {
    auto p = dynamic_cast<const PointerType*>(other);
    if (!p) return false;
    return Type::areSame(p->to().get(), to().get());
}   

void FunctionType::dump(std::ostream &os) const {
    dumpQualifiers(os);
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
    if (!Type::areSame(f->returnType().get(), returnType().get())) return false;
    if (f->args().size() != args().size()) return false;
    for (size_t i = 0; i < args().size(); ++i)
        if (!Type::areSame(f->args()[i], args()[i])) return false;
    return true;
}

void PackType::dump(std::ostream &os) const {
    os << BLUE << "(arg pack)" << RST;
}

bool PackType::isSameAs(const Type *o) const {
    return o->isPack();
}

void TypeType::dump(std::ostream &os) const {
    os << RED << "type" << RST;
}

bool TypeType::isSameAs(const Type *o) const {
    return o->isType();
}

void NamedType::dump(std::ostream &os) const {
    dumpQualifiers(os);
    os << RED << "type " << CYAN << name() << RST;
    if (isResolved())
        os << DGRAY << " = " << RST << *resolved();
    else if (decl() != nullptr)
        os << DGRAY << " = (decl found)" << RST;
    else
        os << DGRAY << " = (unresolved)" << RST;
}

bool NamedType::isSameAs(const Type *o) const {
    assert(false); // they must be unwrapped first
    return false;
}

};
