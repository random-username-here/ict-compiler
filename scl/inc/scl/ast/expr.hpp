#pragma once
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "misclib/parse.hpp"
#include "misclib/tree.hpp"
#include "scl/scope.hpp"

namespace scl {

using misc::UPtr;

class Expr;
class Binary;
class Unary;
class Number;

enum BinaryKind {
    BIN_UNKNOWN = 0,
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_MOD,
    BIN_LT,
    BIN_LE,
    BIN_GT,
    BIN_GE,
    BIN_EQ,
    BIN_NEQ,
    BIN_COMMA,
    BIN_SPACE, // empty space operator (`a b`), used as call/multiplication/..., depending on type
};

ict::View binKind2str(BinaryKind k);

enum UnaryKind {
    UN_UNKNOWN = 0,
    UN_NEG,
    UN_REF,
    UN_DEREF,
};

ict::View unKind2str(UnaryKind k);

class Expr : public misc::Item<Expr> {
public:
    virtual void dump(std::ostream &os) const = 0;
    virtual ~Expr() {};
};

class Binary : public Expr {
    BinaryKind m_kind;
    misc::Slot<Binary, Expr> m_left, m_right;
public:
    Binary() :m_left(this), m_right(this), m_kind(BIN_UNKNOWN) {}
    Binary(BinaryKind kind, UPtr<Expr> &&l, UPtr<Expr> &&r)
        :m_left(this, std::move(l)), m_right(this, std::move(r)), m_kind(kind) {}
    MISC_CREATEFUNC(Binary);

    void dump(std::ostream &os) const override;
    BinaryKind kind() const { return m_kind; }
    auto &left() { return m_left; }
    auto &right() { return m_right; }
    auto &left() const { return m_left; }
    auto &right() const { return m_right; }
};

class Unary : public Expr {
    UnaryKind m_kind;
    misc::Slot<Unary, Expr> m_val;
public:
    Unary() :m_kind(UN_UNKNOWN), m_val(this) {}
    Unary(UnaryKind k, UPtr<Expr> &&v)  :m_kind(k), m_val(this, std::move(v)) {}
    MISC_CREATEFUNC(Unary);
    
    void dump(std::ostream &os) const override;
    UnaryKind kind() const { return m_kind; }
    auto &val() { return m_val; }
    auto &val() const { return m_val; }
};

class Number : public Expr {
    ict::Integer m_val;
public:
    Number(ict::Integer i = 0) :m_val(i) {}
    MISC_CREATEFUNC(Number);
    auto val() const { return m_val; }
    void dump(std::ostream &os) const override;
};

class Name : public Expr {
    std::string m_name;
    ScopeItem *m_item = nullptr;
    misc::Token m_tok;
public:
    Name(misc::Token tok) :m_name(tok.view), m_tok(tok) {}
    Name(ict::View n = "") :m_name(n) {}
    MISC_CREATEFUNC(Name);

    misc::Token token() const { return m_tok; }
    bool hasToken() const { return m_tok.type != misc::TOK_EOF; }

    ScopeItem *scopeItem() { return m_item; }
    const ScopeItem *scopeItem() const { return m_item; }
    void setScopeItem(ScopeItem *it) { m_item = it; }

    auto name() const { return m_name; }
    void dump(std::ostream &os) const override;
};

class ArgPack : public Expr {
    misc::SlotVector<ArgPack, Expr> m_items;
public:
    ArgPack() :m_items(this) {}
    MISC_CREATEFUNC(ArgPack);

    const auto &items() const { return m_items; }
    auto &items() { return m_items; }

    void dump(std::ostream &os) const override;
};


};
