#pragma once
#include "ict/mod.hpp"
#include "misclib/parse.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/module.hpp"
#include <functional>
#include <initializer_list>
namespace scl {

using misc::View;

/// Take a type.
UPtr<Type> parseType(View &source);

/// Take one item in expression, which is not an operator
/// This allows braced expressions and labels
UPtr<Expr> parseExprItem(View &source);

/// Parse expression until something
UPtr<Expr> parseExpr(View &source, std::initializer_list<misc::TokenType> terminals);
inline UPtr<Expr> parseExpr(View &source, misc::TokenType terminal) { return parseExpr(source, {terminal}); }

/// Parse something like variable declarations (like `x int64 = 3, y *void`)
/// This thing occurs frequently, for example in var decls, arguments, structs
void parseVarDeclLike(View &source, misc::TokenType separator, misc::TokenType end, 
        std::function<void(misc::Token name, UPtr<Type> &&type, UPtr<Expr> &&initializer)>);

/// Parse one statement (expr, var decl, if, block)
UPtr<Statement> parseStatement(View &source);

/// Parse one top-level thing.
/// There can be include directives here, so sometimes we get nothing/many entries here.
bool parseTopLevel(View &source, Module *into, const std::filesystem::path &file);

void parseFile(ict::SourceFile *file, Module *into);

};
