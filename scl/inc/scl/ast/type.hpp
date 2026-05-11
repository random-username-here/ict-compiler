#pragma once
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "misclib/tree.hpp"
#include <optional>
namespace scl {

using misc::UPtr;

class Type : public misc::Item<Type> {
public:
    virtual UPtr<Type> copy() const = 0;
    virtual UPtr<ict::Type> toICT() const = 0;
    virtual void dump(std::ostream &o) const = 0;
    virtual bool isSameAs(const Type *other) const = 0;

    virtual size_t byteSize() const = 0;
    virtual size_t alignment() const = 0;
    virtual bool isComplete() const = 0;

    bool isInteger() const;
    bool isVoid() const;
    bool isPointer() const;
    bool isFunction() const;
    bool isFunctionPtr() const;
    bool isPack() const;
};

#define SCL_FOR_EACH_TYPE(X)\
        X(INT64, "int64", 8, ict::Type::i64_t())\
        X(BOOL, "bool", 1, ict::Type::i8_t()) X(VOID, "void", 0, ict::Type::void_t())

class PrimitiveType : public Type {
public:

    enum Kind {
#define X(id, sn, sz, ic) id,
SCL_FOR_EACH_TYPE(X)
#undef X
    };

private:
    Kind m_kind;
public:

    static std::optional<Kind> kindFromString(misc::View name);

    PrimitiveType(Kind k) :m_kind(k) {}
    MISC_CREATEFUNC(PrimitiveType);

    Kind kind() const { return m_kind; }

    virtual UPtr<Type> copy() const { return PrimitiveType::create(kind()); };
    virtual UPtr<ict::Type> toICT() const;
    virtual void dump(std::ostream &o) const;
    virtual bool isSameAs(const Type *other) const;

    virtual size_t byteSize() const;
    virtual size_t alignment() const;
    virtual bool isComplete() const { return m_kind != VOID; }

};

class PointerType : public Type {
    misc::Slot<PointerType, Type> m_to;
public:
    PointerType(UPtr<Type> &&v) :m_to(this, std::move(v)) {}
    MISC_CREATEFUNC(PointerType);

    auto &to() { return m_to; }
    auto &to() const { return m_to; }
    virtual UPtr<ict::Type> toICT() const { return ict::Type::ptr_t(); };
    virtual UPtr<Type> copy() const { return PointerType::create(to()->copy()); };
    virtual void dump(std::ostream &o) const;
    virtual bool isSameAs(const Type *other) const;

    virtual size_t byteSize() const { return 8; } // TODO: get this from target arch
    virtual size_t alignment() const { return 8; }
    virtual bool isComplete() const { return true; }
};

class FunctionType : public Type {
    misc::Slot<FunctionType, Type> m_returnType;
    misc::SlotVector<FunctionType, Type> m_args;
public:
    template<typename ...Args>
    FunctionType(UPtr<Type> &&rt, Args &&...args) 
        :m_returnType(this, std::move(rt)), m_args(this, std::forward<Args>(args)...) 
    {}
    FunctionType() :m_returnType(this), m_args(this) {}
    MISC_CREATEFUNC(FunctionType);

    auto &args() { return m_args; } auto &args() const { return m_args; }
    auto &returnType() { return m_returnType; } auto &returnType() const { return m_returnType; }

    virtual UPtr<ict::Type> toICT() const { return nullptr; };
    virtual UPtr<Type> copy() const { 
        auto ft = FunctionType::create();
        ft->returnType() = returnType()->copy();
        for (auto i : args())
            ft->args().push(i->copy());
        return ft;
    };

    virtual void dump(std::ostream &o) const;
    virtual bool isSameAs(const Type *other) const;

    virtual size_t byteSize() const { return 0; }
    virtual size_t alignment() const { return 1; }
    virtual bool isComplete() const { return false; }
};

class PackType : public Type {
public:
    PackType() {}
    MISC_CREATEFUNC(PackType);

    virtual UPtr<ict::Type> toICT() const { return nullptr; };
    virtual UPtr<Type> copy() const { return PackType::create(); }

    virtual void dump(std::ostream &o) const;
    virtual bool isSameAs(const Type *other) const;

    virtual size_t byteSize() const { return 0; }
    virtual size_t alignment() const { return 1; }
    virtual bool isComplete() const { return false; }


};

};
