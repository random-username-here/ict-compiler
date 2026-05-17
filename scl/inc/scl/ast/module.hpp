#pragma once
#include "misclib/defs.hpp"
#include "misclib/tree.hpp"
#include "scl/ast/basic.hpp"
#include "scl/ast/block.hpp"

namespace scl {

class Module;
class TopLevel;
class Function;
class FunctionArg;

class Module {
    misc::SlotVector<Module, TopLevel> m_entries;
public:
    Module() :m_entries(this) {}
    MISC_CREATEFUNC(Module);

    auto &entries() { return m_entries; }
    auto &entries() const { return m_entries; }

    void dump(std::ostream &os) const;
};

class TopLevel : public MISC_ITEM_IN(TopLevel, &Module::entries) {
public:
    virtual void dump(std::ostream &os) const = 0;
    virtual ~TopLevel() {}
};

class Function : public TopLevel, public GlobalDecl {
    misc::Slot<Function, Statement> m_body;
    misc::SlotVector<Function, FunctionArg> m_args;
    misc::Slot<Function, Type> m_returnType;
public:
    Function(misc::Token name) :m_body(this), m_args(this), m_returnType(this), GlobalDecl(name) {}
    MISC_CREATEFUNC(Function);

    auto &body() { return m_body; } auto &body() const { return m_body; }
    auto &args() { return m_args; } auto &args() const { return m_args; }
    auto &returnType() { return m_returnType; } auto &returnType() const { return m_returnType; }

    void genDeclType();
    void dump(std::ostream &os) const override;
};

class FunctionArg : public MISC_ITEM_IN(Function, &Function::args), public LocalDecl {
public:
    FunctionArg(misc::Token name, UPtr<Type> &&type = nullptr) :LocalDecl(name, std::move(type)) {}
    MISC_CREATEFUNC(FunctionArg);

    void dump(std::ostream &os) const;
};

class TypeDef : public TopLevel, public TypeDecl {
public:
    using TypeDecl::TypeDecl;
    MISC_CREATEFUNC(TypeDef);

    void dump(std::ostream &os) const override;
};

class GlobalVar;

class GlobalVarBlock : public TopLevel {
    misc::SlotList<GlobalVarBlock, GlobalVar> m_vars;
public:
    GlobalVarBlock() :m_vars(this) {}
    MISC_CREATEFUNC(GlobalVarBlock);

    auto &vars() { return m_vars; } auto &vars() const { return m_vars; }
    void dump(std::ostream &os) const override;
};

class GlobalVar : public GlobalDecl, public MISC_ITEM_IN(GlobalVar, &GlobalVarBlock::vars) {
    misc::Slot<GlobalVar, Expr> m_initExpr;
public:
    GlobalVar(misc::Token name, UPtr<Type> &&type, UPtr<Expr> &&init = nullptr) 
        :m_initExpr(this, std::move(init)), GlobalDecl(name, std::move(type)) {}
    MISC_CREATEFUNC(GlobalVar);

    auto &initExpr() { return m_initExpr; } auto &initExpr() const { return m_initExpr; }
    void dump(std::ostream &os) const;
};

};
