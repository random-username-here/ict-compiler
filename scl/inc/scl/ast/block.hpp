#pragma once
#include "misclib/defs.hpp"
#include "misclib/parse.hpp"
#include "misclib/tree.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/basic.hpp"
#include "scl/ast/type.hpp"
#include <ostream>

namespace scl {

class Block; // something wrapped in {}
class Statement; // something inside block
class ExprInBlock; // raw expression in block
class VarDecl; // variable declaration

class Statement : public misc::Item<Statement, misc::SlotVector<Block, Statement>>, public misc::WithToken {
public:
    Statement(misc::Token tok) :WithToken(tok) {}
    virtual void dump(std::ostream &os) const = 0;
};

// may have nullptr parent, in that case it is top function block
class Block : public misc::Item<Block>, public Statement {
    misc::SlotVector<Block, Statement> m_items;
public:

    Block(misc::Token tok) :Statement(tok), m_items(this) {}
    MISC_CREATEFUNC(Block);

    auto &items() { return m_items; }
    auto &items() const { return m_items; }
    void dump(std::ostream &os) const override;
};


class ExprInBlock : public Statement {
    misc::Slot<ExprInBlock, Expr> m_expr;
public:
    ExprInBlock(misc::Token tok, misc::UPtr<Expr> &&e) :Statement(tok), m_expr(this, std::move(e)) {}
    MISC_CREATEFUNC(ExprInBlock);

    auto &expr() { return m_expr; }
    auto &expr() const { return m_expr; }
    void dump(std::ostream &os) const override;
};

/** Variable declaration statement: `var x int, y float = 3.14;` */
class VarDeclStatement : public Statement {
    misc::SlotVector<VarDeclStatement, VarDecl> m_decls;
public:
    VarDeclStatement(misc::Token tok) :Statement(tok), m_decls(this) {}
    MISC_CREATEFUNC(VarDeclStatement);

    auto &decls() { return m_decls; }
    auto &decls() const { return m_decls; }
    void dump(std::ostream &os) const override;
};

/** Single variable declaration */
class VarDecl : public MISC_ITEM_IN(VarDecl, &VarDeclStatement::decls), public LocalDecl {
    misc::Slot<VarDecl, Expr> m_initExpr;
public:
    VarDecl(misc::Token name, UPtr<Type> &&type, UPtr<Expr> &&init) 
        :m_initExpr(this, std::move(init)), LocalDecl(name, std::move(type)) {}
    MISC_CREATEFUNC(VarDecl);

    auto &initExpr() { return m_initExpr; }
    auto &initExpr() const { return m_initExpr; }
    void dump(std::ostream &os) const override;
};

class IfElse : public Statement {
    misc::Slot<IfElse, Expr> m_cond;
    misc::Slot<IfElse, Statement> m_then;
    misc::Slot<IfElse, Statement> m_otherwise;
public:
    IfElse(misc::Token tok, UPtr<Expr> &&cond, UPtr<Statement> &&then, UPtr<Statement> &&otherwise)
        :Statement(tok), m_cond(this, std::move(cond)), m_then(this, std::move(then)), m_otherwise(this, std::move(otherwise)) {}
    MISC_CREATEFUNC(IfElse);

    auto &cond() { return m_cond; }
    auto &cond() const { return m_cond; }
    auto &then() { return m_then; }
    auto &then() const { return m_then; }
    auto &otherwise() { return m_otherwise; }
    auto &otherwise() const { return m_otherwise; }
    void dump(std::ostream &os) const override;
};

};
