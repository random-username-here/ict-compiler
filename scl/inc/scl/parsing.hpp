#pragma once
#include "misclib/parse.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/block.hpp"
#include <initializer_list>
namespace scl {

using misc::View;

/// Take one item in expression, which is not an operator
/// This allows braced expressions and labels
UPtr<Expr> parseExprItem(View &source);

/// Parse expression until something
UPtr<Expr> parseExpr(View &source, std::initializer_list<misc::TokenType> terminals);
inline UPtr<Expr> parseExpr(View &source, misc::TokenType terminal) { return parseExpr(source, {terminal}); }

/// Parse one statement (expr, var decl, if, block)
UPtr<Statement> parseStatement(View &source);

};
