/**
 * Unlike ict, all passes are built-in here
 * TODO: move them to plugins also
 */
#pragma once

#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/module.hpp"
#include "scl/scope.hpp"

namespace scl {


void resolveScopes(Statement *stmnt, Scope *parent);
void resolveScopes(Expr *expr, Scope *parent);
void resolveScopes(Module *mod, Scope *parent);

void resolveTypes(Statement *smnt);
void resolveTypes(Expr *expr);
void resolveTypes(Module *mod);

};
