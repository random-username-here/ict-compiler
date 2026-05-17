#pragma once
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "misclib/parse.hpp"
#include "misclib/tree.hpp"
#include "scl/ast/basic.hpp"
#include "scl/ast/type.hpp"
#include <cstddef>

namespace scl {

using misc::UPtr;

class Expr;
class Binary;
class Unary;
class Number;

enum BinaryKind {
    BIN_UNKNOWN = 0,

    // Simple math
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_MOD, 
    BIN_RSH,
    BIN_LSH,
    BIN_BOR,
    BIN_BAND,
    BIN_BXOR,

    // Comparsions
    BIN_LT,
    BIN_LE,
    BIN_GT,
    BIN_GE,
    BIN_EQ,
    BIN_NEQ,

    // Boolean
    BIN_OR,
    BIN_AND,

    // Inplace
    BIN_IADD,
    BIN_ISUB,
    BIN_IMUL,
    BIN_IDIV,
    BIN_IMOD,
    BIN_IRSH,
    BIN_ILSH,
    BIN_IBXOR,
    BIN_IBAND,
    BIN_IBOR,
    // TODO: &&= ?

    // Special
    BIN_COMMA,
    BIN_ASSIGN,
    BIN_SPACE, // empty space operator (`a b`), used as call/multiplication/..., depending on type
};

ict::View binKind2str(BinaryKind k);

enum UnaryKind {
    UN_UNKNOWN = 0,

    UN_NEG, // -
    UN_NOT, // !
    UN_INV, // ~

    UN_INC_PRE,
    UN_DEC_PRE,
    UN_INC_POST,
    UN_DEC_POST,

    UN_REF,
    UN_DEREF,
};

ict::View unKind2str(UnaryKind k);

class Expr : public misc::Item<Expr>, public misc::WithToken {
    misc::Slot<Expr, Type> m_type;
public:
    Expr(misc::Token tok) :WithToken(tok), m_type(this) {}
    Expr(std::nullptr_t) :WithToken(nullptr), m_type(this) {}
    auto &type() { return m_type; }
    auto &type() const { return m_type; }
    virtual void dump(std::ostream &os) const = 0;
    virtual ~Expr() {};
};

class Binary : public Expr {
    BinaryKind m_kind;
    misc::Slot<Binary, Expr> m_left, m_right;
public:
    Binary(misc::Token tok, BinaryKind kind, UPtr<Expr> &&l, UPtr<Expr> &&r)
        :Expr(tok), m_left(this, std::move(l)), m_right(this, std::move(r)), m_kind(kind) {}
    Binary(BinaryKind kind, UPtr<Expr> &&l, UPtr<Expr> &&r)
        :Expr(nullptr), m_left(this, std::move(l)), m_right(this, std::move(r)), m_kind(kind) {}
    MISC_CREATEFUNC(Binary);

    void setKind(BinaryKind k) { m_kind = k; } // used to convert space to multiplication

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
    Unary(misc::Token tok, UnaryKind k, UPtr<Expr> &&v)  :Expr(tok), m_kind(k), m_val(this, std::move(v)) {}
    MISC_CREATEFUNC(Unary);
   
    void setKind(UnaryKind k) { m_kind = k; }

    void dump(std::ostream &os) const override;
    UnaryKind kind() const { return m_kind; }
    auto &val() { return m_val; }
    auto &val() const { return m_val; }
};

class Number : public Expr {
    ict::Integer m_val;
public:
    Number(ict::Integer v) :Expr(nullptr), m_val(v) {}
    Number(misc::Token tok) :Expr(tok), m_val(tok.decodeNum()) {}
    MISC_CREATEFUNC(Number);

    auto val() const { return m_val; }
    void dump(std::ostream &os) const override;
};

class String : public Expr {
    std::string m_val;
public:
    String(misc::View v) :Expr(nullptr), m_val(v) {}
    String(misc::Token tok) :Expr(tok), m_val(tok.decodeStr()) {}
    MISC_CREATEFUNC(String);

    auto val() const { return m_val; }
    void dump(std::ostream &os) const override;
};


class Name : public Expr {
    Decl *m_item = nullptr;
public:
    Name(misc::Token tok) :Expr(tok) {}
    MISC_CREATEFUNC(Name);

    Decl *decl() { return m_item; }
    const Decl *decl() const { return m_item; }
    void setDecl(Decl *d) { m_item = d; }

    auto name() const { return token().view; }
    void dump(std::ostream &os) const override;
};

class ArgPack : public Expr {
    misc::SlotVector<ArgPack, Expr> m_items;
public:
    template<typename ...Args>
    ArgPack(misc::Token tok, Args &&...args) :Expr(tok), m_items(this, std::forward<Args>(args)...) {}
    MISC_CREATEFUNC(ArgPack);

    const auto &items() const { return m_items; }
    auto &items() { return m_items; }

    void dump(std::ostream &os) const override;
};

};
