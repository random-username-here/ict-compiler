#include "ict/ir.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"

namespace scl {

struct IRGenCtx {
    ict::BasicBlock *curBlock;

    ict::FunctionImpl *func() { return curBlock->parent(); }
};

ict::Operation *irExpr(IRGenCtx *ctx, Expr *expr);
void irStmnt(IRGenCtx *ctx, Statement *stmnt);

};
