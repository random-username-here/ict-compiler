/**
 * Unlike ict, all passes are built-in here
 * TODO: move them to plugins also
 */
#pragma once

#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/scope.hpp"

namespace scl {

void resolveScopes(Statement *stmnt, Scope *parent);
void resolveScopes(Expr *expr, Scope *parent);

};
