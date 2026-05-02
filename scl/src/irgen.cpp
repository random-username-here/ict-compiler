#include "scl/irgen.hpp"
#include "ict/ir.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include <stdexcept>

namespace scl {

ict::Operation *irExpr(IRGenCtx *ctx, Expr *expr) {
    auto blk = ctx->curBlock;
    if (auto num = dynamic_cast<Number*>(expr)) {
        return blk->operations().createEnd(ict::OP_CONST, nullptr, num->val());
    } else if (auto unary = dynamic_cast<Unary*>(expr)) {
        switch (unary->kind()) {
            case UN_UNKNOWN: throw std::runtime_error("UN_UNKNOWN found");
            case UN_NEG: {
                auto zero = blk->operations().createEnd(ict::OP_CONST, nullptr, 0);
                auto val = irExpr(ctx, unary->val().get());
                return blk->operations().createEnd(ict::OP_SUB, nullptr, zero, val);
            };
            case UN_REF: {
                // TODO: do it
                throw std::runtime_error("Cannot to take reference");
            }
            case UN_DEREF: {
                // TODO: types
                auto addr = irExpr(ctx, unary->val().get());
                return blk->operations().createEnd(ict::OP_LOAD, ict::Type::i64_t(), addr);
            }
        }
    } else if (auto binary = dynamic_cast<Binary*>(expr)) {
        int kind = 0; 
        auto left = irExpr(ctx, binary->left().get());
        auto right = irExpr(ctx, binary->right().get());
        // TODO: run right only if left is false/true in OR/AND
        switch (binary->kind()) {
            case BIN_UNKNOWN: throw std::runtime_error("BIN_UNKNOWN found");
            case BIN_SPACE: throw std::runtime_error("Space operator should not reach irgen");
            case BIN_COMMA: return right;
            case BIN_ADD: kind = ict::OP_ADD; break;
            case BIN_SUB: kind = ict::OP_SUB; break;
            case BIN_MUL: kind = ict::OP_MUL; break;
            case BIN_DIV: kind = ict::OP_DIV; break;
            case BIN_MOD: kind = ict::OP_MOD; break;
            case BIN_LT:  kind = ict::OP_LT; break;
            case BIN_LE:  kind = ict::OP_LE; break;
            case BIN_GT:  kind = ict::OP_GT; break;
            case BIN_GE:  kind = ict::OP_GE; break;
            case BIN_EQ:  kind = ict::OP_EQ; break;
            case BIN_NEQ: kind = ict::OP_NEQ; break;
        }
        return blk->operations().createEnd(kind, nullptr, left, right);
    } else if (auto name = dynamic_cast<Name*>(expr)) {
        assert(name->scopeItem() != nullptr);
        assert(name->scopeItem()->addr() != nullptr);
        return blk->operations().createEnd(ict::OP_LOAD, ict::Type::i64_t(), name->scopeItem()->addr());
    } else {
        throw std::runtime_error("Unknown expression type!");
    }
}

void irStmnt(IRGenCtx *ctx, Statement *stmnt) {
    if (auto block = dynamic_cast<Block*>(stmnt)) {
        for (auto i : block->items())
            irStmnt(ctx, i);
    } else if (auto expr = dynamic_cast<ExprInBlock*>(stmnt)) {
        irExpr(ctx, expr->expr().get());
    } else if (auto decl = dynamic_cast<VarDeclStatement*>(stmnt)) {
        for (auto i : decl->decls()) {
            auto alloc = ctx->curBlock->operations().createEnd(ict::OP_ALLOCA, ict::Type::i64_t());
            i->scopeItem()->setAddr(alloc);
            if (i->initExpr()) {
                auto val = irExpr(ctx, i->initExpr().get());
                ctx->curBlock->operations().createEnd(ict::OP_STORE, ict::Type::i64_t(), alloc, val);
            }
        }
    } else if (auto ifElse = dynamic_cast<IfElse*>(stmnt)) {
        auto cond = irExpr(ctx, ifElse->cond().get());
        auto then = ctx->func()->blocks().createAfter(ctx->curBlock);
        auto followup = ctx->func()->blocks().createAfter(then);
        if (ifElse->otherwise()) {
            //
            // cur -- then --- followup
            //   \_ otherwise _/
            //
            auto otherwise = ctx->func()->blocks().createBefore(followup);
            ctx->curBlock->operations().createEnd(ict::OP_BC, nullptr, cond, then, otherwise);
            ctx->curBlock = then;
            irStmnt(ctx, ifElse->then().get()); // curBlock may change
            ctx->curBlock->operations().createEnd(ict::OP_BR, nullptr, followup);
            ctx->curBlock = otherwise;
            irStmnt(ctx, ifElse->otherwise().get()); // curBlock may change
            ctx->curBlock->operations().createEnd(ict::OP_BR, nullptr, followup);
            ctx->curBlock = followup;
        } else {
            //
            // cur -- then -- followup
            //   \____________/
            //
            ctx->curBlock->operations().createEnd(ict::OP_BC, nullptr, cond, then, followup);
            ctx->curBlock = then;
            irStmnt(ctx, ifElse->then().get()); // curBlock may change
            ctx->curBlock->operations().createEnd(ict::OP_BR, nullptr, followup);
            ctx->curBlock = followup;
        }
    } else {
        throw std::runtime_error("Unknown statement type!");
    }
}

};
