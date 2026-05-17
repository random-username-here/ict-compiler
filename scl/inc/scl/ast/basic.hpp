#pragma once
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "misclib/parse.hpp"
//#include "scl/ast/type.hpp"

namespace scl {

class Type;

class Decl : public misc::WithToken {
private:
    misc::Slot<Decl, Type> m_type;

public:
    Decl(misc::Token name, ict::UPtr<Type> &&t = nullptr) :WithToken(name), m_type(this, std::move(t)) {}

    misc::View name() const { return token().view; }

    auto &type() { return m_type; }
    auto &type() const { return m_type; }
    
    virtual ict::Operation *genAddr(ict::BasicBlock *inside) = 0;
};

class LocalDecl : public Decl {
    ict::Operation *m_op = nullptr;
public:
    using Decl::Decl;

    void setAddr(ict::Operation *op) { m_op = op; }
    ict::Operation *genAddr(ict::BasicBlock *_) override { return m_op; }
};

class GlobalDecl : public Decl {
    ict::TopDecl *m_decl = nullptr;
public:
    using Decl::Decl;
    void setIctDecl(ict::TopDecl *decl) { m_decl = decl; }
    ict::Operation *genAddr(ict::BasicBlock *blk) override { 
        return blk->operations().createEnd(ict::OP_GLOBALPTR, nullptr, m_decl); 
    }
};

class Type;

class TypeDecl : public Decl {
    misc::Slot<TypeDecl, Type> m_aliased;
public:
    TypeDecl(misc::Token name, ict::UPtr<Type> &&t); 

    auto &aliasedType() { return m_aliased; } 
    auto &aliasedType() const { return m_aliased; } 

    virtual ict::Operation *genAddr(ict::BasicBlock *inside) { return nullptr; }
};

};
