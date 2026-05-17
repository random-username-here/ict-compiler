#include "ict/ir.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/module.hpp"
#include <cstddef>

namespace scl {

struct IRGenCtx {
    ict::BasicBlock *curBlock;

    ict::FunctionImpl *func() { return curBlock->parent(); }
    ict::FunctionDecl *funcDecl() { return func()->decl(); }
    ict::Module *module() { return func()->parent(); }

    bool blockIsEnded() const {
        return curBlock->operations().size() && curBlock->operations().last()->isTerminal();
    }

    template<typename ...Args>
    ict::Operation *emit(Args &&...args) {
        return curBlock->operations().createEnd(std::forward<Args>(args)...);
    }

    ict::BasicBlock *newBlock() {
        return func()->blocks().createAfter(curBlock);
    }

    ict::BasicBlock *newBlockAfter(ict::BasicBlock *blk) {
        return func()->blocks().createAfter(blk);
    }
};

struct IrExprResult {
    ict::Operation *value, *pointer = nullptr;
    IrExprResult(std::nullptr_t) :value(nullptr) {}
    IrExprResult(ict::Operation *val) :value(val) {}
    IrExprResult(ict::Operation *val, ict::Operation *ptr) :value(val), pointer(ptr) {}
};

IrExprResult irExpr(IRGenCtx *ctx, Expr *expr, bool valueNeeded = true);
void irStmnt(IRGenCtx *ctx, Statement *stmnt);
void irModule(ict::Module *into, Module *mod);

};
