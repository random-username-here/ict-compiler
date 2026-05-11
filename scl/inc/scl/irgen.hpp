#include "ict/ir.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/module.hpp"

namespace scl {

struct IRGenCtx {
    ict::BasicBlock *curBlock;

    ict::FunctionImpl *func() { return curBlock->parent(); }
};

ict::Operation *irExpr(IRGenCtx *ctx, Expr *expr);
void irStmnt(IRGenCtx *ctx, Statement *stmnt);
void irModule(ict::Module *into, Module *mod);

};
