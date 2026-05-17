#include "scl/irgen.hpp"
#include "ict/ir.hpp"
#include "misclib/parse.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/module.hpp"
#include "scl/ast/type.hpp"
#include <stdexcept>

namespace scl {

static ict::Operation *l_genAssign(IRGenCtx *ctx, Binary *binary) {
    auto val = irExpr(ctx, binary->right().get());
    Type *type = nullptr;
    ict::Operation *ptr = nullptr;
    if (auto name = dynamic_cast<Name*>(binary->left().get())) {
        ptr = name->decl()->genAddr(ctx->curBlock);
        type = name->decl()->type().get();
        if (!ptr)
            throw misc::SourceError(name->token(), "Cannot assign to non-locals for now");
    } else if (auto deref = dynamic_cast<Unary*>(binary->left().get())) {
        assert(deref->kind() == UN_DEREF);
        ptr = irExpr(ctx, deref->val().get());
        auto pt = dynamic_cast<PointerType*>(deref->val()->type().get());
        assert(pt);
        type = pt->to().get();
    } else {
        assert(false); // somehow this missed type checking?
    }
    auto t = type->toICT();
    assert(t != nullptr);
    ctx->curBlock->operations().createEnd(ict::OP_STORE, std::move(t), ptr, val);
    return val;
}

static ict::Operation *l_genCall(IRGenCtx *ctx, Binary *binary) {
    assert(binary->left()->type()->isFunctionPtr());
    std::vector<ict::Operation*> args;
    args.push_back(irExpr(ctx, binary->left().get())); // get addr
    auto pack = static_cast<ArgPack*>(binary->right().get());
    for (auto i : pack->items())
        args.push_back(irExpr(ctx, i));
    auto c = ctx->curBlock->operations().createEnd(ict::OP_CALL, binary->type()->toICT());
    for (auto i : args)
        c->args().createEnd<ict::VRegArg>(i);
    return c;
}

ict::Operation *irExpr(IRGenCtx *ctx, Expr *expr) {
    auto blk = ctx->curBlock;
    if (auto num = dynamic_cast<Number*>(expr)) {
        return blk->operations().createEnd(ict::OP_CONST, nullptr, num->val());
    } else if (auto str = dynamic_cast<String*>(expr)) {
        auto blob = ctx->func()->parent()->createString(str->val());
        return blk->operations().createEnd(ict::OP_GLOBALPTR, nullptr, blob);
    } else if (auto unary = dynamic_cast<Unary*>(expr)) {
        switch (unary->kind()) {
            case UN_UNKNOWN: throw std::runtime_error("UN_UNKNOWN found");
            case UN_NEG: {
                auto zero = blk->operations().createEnd(ict::OP_CONST, nullptr, 0);
                auto val = irExpr(ctx, unary->val().get());
                return blk->operations().createEnd(ict::OP_SUB, nullptr, zero, val);
            };
            case UN_REF: {
                auto name = dynamic_cast<Name*>(unary->val().get());
                assert(name);
                return name->decl()->genAddr(ctx->curBlock);
            }
            case UN_DEREF: {
                // TODO: types
                auto ptr = dynamic_cast<PointerType*>(unary->val()->type().get());
                assert(ptr);
                auto it = ptr->to()->toICT();
                if (!it)
                    throw misc::SourceError(unary->token(), "Currently non-ict-convertible types cannot be dereferenced");
                auto addr = irExpr(ctx, unary->val().get());
                return blk->operations().createEnd(ict::OP_LOAD, std::move(it), addr);
            }
        }
    } else if (auto binary = dynamic_cast<Binary*>(expr)) {
        if (binary->kind() == BIN_ASSIGN)
            return l_genAssign(ctx, binary);
        else if (binary->kind() == BIN_SPACE)
            return l_genCall(ctx, binary);

        int kind = 0; 
        auto left = irExpr(ctx, binary->left().get());
        auto right = irExpr(ctx, binary->right().get());
        if (binary->kind() == BIN_ADD || binary->kind() == BIN_SUB) {
            if (binary->left()->type()->isPointer()) {
                auto size = binary->left()->type()->as<PointerType>()->to()->byteSize();
                if (size == 0) size = 1;
                right = blk->operations().createEnd(ict::OP_MUL, nullptr, right, size);
            } else if (binary->right()->type()->isPointer()) {
                auto size = binary->left()->type()->as<PointerType>()->to()->byteSize();
                if (size == 0) size = 1;
                left = blk->operations().createEnd(ict::OP_MUL, nullptr, left, size);
            }
        }
        // TODO: run right only if left is false/true in OR/AND
        switch (binary->kind()) {
            case BIN_ASSIGN: assert(false);
            case BIN_SPACE: assert(false);
            case BIN_UNKNOWN: throw std::runtime_error("BIN_UNKNOWN found");

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
            case BIN_LSH: kind = ict::OP_LSH; break;
            case BIN_RSH: kind = ict::OP_RSH; break;
            case BIN_BAND: kind = ict::OP_AND; break;
            case BIN_BOR: kind = ict::OP_OR; break;
            case BIN_BXOR: kind = ict::OP_XOR; break;
        }
        return blk->operations().createEnd(kind, nullptr, left, right);
    } else if (auto name = dynamic_cast<Name*>(expr)) {
        assert(name->decl() != nullptr);
        auto addr = name->decl()->genAddr(ctx->curBlock);
        assert(addr != nullptr);
        return blk->operations().createEnd(ict::OP_LOAD, name->type()->toICT(), addr);
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
            auto alloc = ctx->curBlock->operations().createEnd(ict::OP_ALLOCA, i->type()->toICT());
            alloc->addTag(ict::Tag(std::string("var.") + std::string(i->name())));
            i->setAddr(alloc);
            if (i->initExpr()) {
                auto val = irExpr(ctx, i->initExpr().get());
                ctx->curBlock->operations().createEnd(ict::OP_STORE, i->type()->toICT(), alloc, val);
            }
        }
    } else if (auto ifElse = dynamic_cast<IfElse*>(stmnt)) {
        auto cond = irExpr(ctx, ifElse->cond().get());
        auto then = ctx->func()->blocks().createAfter(ctx->curBlock);
        auto followup = ctx->func()->blocks().createAfter(then);
        then->addTag("if.then");
        followup->addTag("if.followup");
        if (ifElse->otherwise()) {
            //
            // cur -- then --- followup
            //   \_ otherwise _/
            //
            auto otherwise = ctx->func()->blocks().createBefore(followup);
            otherwise->addTag("if.else");
            ctx->curBlock->operations().createEnd(ict::OP_BC, nullptr, cond, then, otherwise);
            ctx->curBlock = then;
            irStmnt(ctx, ifElse->then().get()); // curBlock may change
            if (!ctx->blockIsEnded())
                ctx->curBlock->operations().createEnd(ict::OP_BR, nullptr, followup);
            ctx->curBlock = otherwise;
            irStmnt(ctx, ifElse->otherwise().get()); // curBlock may change
            if (!ctx->blockIsEnded())
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
            if (!ctx->blockIsEnded())
                ctx->curBlock->operations().createEnd(ict::OP_BR, nullptr, followup);
            ctx->curBlock = followup;
        }
    } else if (auto ret = dynamic_cast<ReturnStatement*>(stmnt)) {
        if (ret->expr()) {
            auto res = irExpr(ctx, ret->expr().get());
            ctx->curBlock->operations().createEnd(ict::OP_RET, nullptr, res);
        } else {
            ctx->curBlock->operations().createEnd(ict::OP_RET, nullptr);
        }
    } else if (auto whl = dynamic_cast<While*>(stmnt)) {
        auto check = ctx->func()->blocks().createAfter(ctx->curBlock);
        auto body = ctx->func()->blocks().createAfter(check);
        auto followup = ctx->func()->blocks().createAfter(body);
        check->addTag("while.check");
        body->addTag("while.body");
        followup->addTag("while.followup");

        ctx->curBlock->operations().createEnd(ict::OP_BR, nullptr, check);

        ctx->curBlock = check;
        auto cond = irExpr(ctx, whl->cond().get());
        ctx->curBlock->operations().createEnd(ict::OP_BC, nullptr, cond, body, followup);

        ctx->curBlock = body;
        irStmnt(ctx, whl->body().get());
        if (!ctx->curBlock->operations().last()->isTerminal())
            ctx->curBlock->operations().createEnd(ict::OP_BR, nullptr, check);

        ctx->curBlock = followup;
    } else {
        throw std::runtime_error("Unknown statement type!");
    }
}

static void l_irFunc(ict::Module *into, Function *func) {
    // TODO: check if function does not exist
    auto o_funcDecl = ict::FunctionDecl::create();
    o_funcDecl->setName(func->name());
    o_funcDecl->retType() = func->returnType()->toICT();
    std::vector<ict::ArgDecl*> argDecls;
    for (auto i : func->args())
        argDecls.push_back(o_funcDecl->args().createEnd(i->type()->toICT(), i->name()));

    auto funcDecl = into->decls().insertEnd(std::move(o_funcDecl));
    func->setIctDecl(funcDecl);

    if (!func->body())
        return;
    auto body = funcDecl->implement();
    auto start = body->createBlock("start");

    for (size_t i = 0; i < func->args().size(); ++i) {
        auto aptr = start->operations().createEnd(ict::OP_ARGPTR, nullptr, argDecls[i]);
        func->args()[i]->setAddr(aptr);
    }

    scl::IRGenCtx ctx = { .curBlock = start };
    // TODO: place return statement
    scl::irStmnt(&ctx, func->body().get());

    if (func->returnType()->isVoid())
        ctx.curBlock->operations().createEnd(ict::OP_RET, nullptr);
}

ict::Integer evalConst(Expr *expr) {
    // TODO: expressions, other constants
    if (auto num = dynamic_cast<Number*>(expr)) {
        return num->val();
    } else {
        throw misc::SourceError(expr->token(), "Constant initializers can only be numbers for now, no expressions");
    }
}

void irModule(ict::Module *into, Module *mod) {
    for (auto tl : mod->entries()) {
        if (auto func = dynamic_cast<Function*>(tl))
            l_irFunc(into, func);
        else if (auto tdecl = dynamic_cast<TypeDef*>(tl)) {
            // skip typedef
        } else if (auto gvb = dynamic_cast<GlobalVarBlock*>(tl)) {
            for (auto i : gvb->vars()) {
                auto decl = into->decls().createEnd<ict::TopDecl>(i->name());
                i->setIctDecl(decl);
                if (!i->type()->isConst()) {
                    if (i->initExpr())
                        throw misc::SourceError(i->initExpr()->token(), "Non-constant globals currently are only zero-initialized");
                    into->impls().createEnd<ict::BssImpl>(decl, i->type()->byteSize());
                } else {
                    if (!i->initExpr())
                        throw misc::SourceError(i->token(), "Uninitialized constant");
                    auto blob = into->impls().createEnd<ict::BlobImpl>(decl);
                    blob->add64(evalConst(i->initExpr().get()));
                }
            }
        } else {
            throw std::runtime_error("TopLevel type not known!");
        }
    }
}

};
