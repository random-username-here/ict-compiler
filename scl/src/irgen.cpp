#include "scl/irgen.hpp"
#include "ict/ir.hpp"
#include "misclib/parse.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/module.hpp"
#include "scl/ast/type.hpp"
#include <stdexcept>

namespace scl {

static ict::Operation *l_irCall(IRGenCtx *ctx, Binary *binary) {
    assert(binary->left()->type()->isFunctionPtr());
    std::vector<ict::Operation*> args;
    args.push_back(irExpr(ctx, binary->left().get()).value); // get addr
    
    auto pack = static_cast<ArgPack*>(binary->right().get());
    for (auto i : pack->items())
        args.push_back(irExpr(ctx, i).value);

    auto c = ctx->emit(ict::OP_CALL, binary->type()->toICT());
    for (auto i : args)
        c->args().createEnd<ict::VRegArg>(i);

    return c;
}

static IrExprResult l_irUnary(IRGenCtx *ctx, Unary *unary, bool valueNeeded) {
    switch (unary->kind()) {
        case UN_UNKNOWN:
            throw std::runtime_error("UN_UNKNOWN found");

        case UN_NEG: {
            // TODO: add ict operator
            if (!valueNeeded) return nullptr;
            auto val = irExpr(ctx, unary->val().get());
            return ctx->emit(ict::OP_SUB, nullptr, 0, val.value);
        };

        case UN_NOT: {
            // TODO: add ict operator
            if (!valueNeeded) return nullptr;
            auto val = irExpr(ctx, unary->val().get());
            // 0 -> 1, 1 -> 0
            return ctx->emit(ict::OP_EQ, nullptr, 0, val.value);
        }

        case UN_INV: {
            // TODO: add ict operator
            uint64_t mask;
            switch (unary->val()->type()->byteSize()) {
                case 1: mask = 0xff; break;
                case 2: mask = 0xffff; break;
                case 4: mask = 0xffffffff; break;
                case 8: mask = 0xffffffffffffffff; break;
                default: throw std::runtime_error("Cannot generate UN_INV: type of invalid size to create mask");
            }
            auto val = irExpr(ctx, unary->val().get());
            return ctx->emit(ict::OP_XOR, nullptr, mask, val.value);
        }

        case UN_DEC_PRE: case UN_DEC_POST:
        case UN_INC_PRE: case UN_INC_POST: {
            auto val = irExpr(ctx, unary->val().get());
            if (!val.pointer)
                throw misc::SourceError(unary->token(), "Operator requires lvalue");

            auto type = unary->val()->type()->toICT();
            if (!type)
                throw misc::SourceError(unary->token(), "Cannot increment non-primitive types");

            ict::Operation *result;
            if (unary->kind() == UN_DEC_PRE || unary->kind() == UN_DEC_POST)
                result = ctx->emit(ict::OP_SUB, nullptr, val.value, 1);
            else
                result = ctx->emit(ict::OP_ADD, nullptr, val.value, 1);

            ctx->emit(ict::OP_STORE, std::move(type), val.pointer, result);

            if (unary->kind() == UN_DEC_PRE || unary->kind() == UN_INC_PRE)
                return IrExprResult(val.pointer, result);
            else
                return val.value;
        }

        case UN_REF: {
            if (!valueNeeded) return nullptr;
            auto res = irExpr(ctx, unary->val().get(), false);
            if (!res.pointer)
                throw misc::SourceError(unary->token(), "Value has no backing pointer");
            return res.pointer;
        }

        case UN_DEREF: {
            auto ptr = dynamic_cast<PointerType*>(unary->val()->type().get());
            assert(ptr);
            auto it = ptr->to()->toICT();
            if (!it && valueNeeded) // TODO: explain this better
                throw misc::SourceError(unary->token(), "Expression of non-primitive type cannot be loaded into register");
            auto addr = irExpr(ctx, unary->val().get());
            return IrExprResult(
                valueNeeded ? ctx->emit(ict::OP_LOAD, std::move(it), addr.value) : nullptr,
                addr.value // we have backing ptr
            );
        }
    }
    throw std::runtime_error("irUnary switch miss");
}

static IrExprResult l_irShortCircuitingBinary(IRGenCtx *ctx, Binary *binary) {
    // NOTE: I want phis here, but i have no time, so we have temp variable
    auto left = irExpr(ctx, binary->left().get());
    
    auto storage = ctx->emit(ict::OP_ALLOCA, ict::Type::i8_t())->addTag("temp.shortcircuit");
    ctx->emit(ict::OP_STORE, ict::Type::i8_t(), storage, left.value);

    auto fullPath = ctx->newBlockAfter(ctx->curBlock);
    auto continuation = ctx->newBlockAfter(fullPath);
    fullPath->addTag("shortcircuit");

    if (binary->kind() == BIN_AND) {
        ctx->emit(ict::OP_BC, nullptr, left.value, fullPath, continuation);
    } else if (binary->kind() == BIN_OR) {
        ctx->emit(ict::OP_BC, nullptr, left.value, continuation, fullPath);
    } else {
        throw std::runtime_error("Bad operation to be generated as short-circuiting");
    }
    
    ctx->curBlock = fullPath;
    auto right = irExpr(ctx, binary->right().get());
    ict::Operation *merge = nullptr;
    if (binary->kind() == BIN_AND) {
        merge = ctx->emit(ict::OP_AND, nullptr, left.value, right.value);
    } else if (binary->kind() == BIN_OR) {
        merge = ctx->emit(ict::OP_OR, nullptr, left.value, right.value);
    } else {
        throw std::runtime_error("Bad operation to be generated as short-circuiting");
    }
    ctx->emit(ict::OP_STORE, ict::Type::i8_t(), storage, merge);
    ctx->emit(ict::OP_BR, nullptr, continuation);

    ctx->curBlock = continuation;
    auto load = ctx->emit(ict::OP_LOAD, ict::Type::i8_t(), storage);
    return load;
}

struct BinaryInfo {
    int ictKind;
    bool isInPlace = false;
    bool isSpecial = false;
    bool isShortCircuiting = false;
    bool hasPointerSizeAdjust = false;
};

static BinaryInfo l_binaryInfo(BinaryKind k) {
  switch (k) {
      case BIN_UNKNOWN: throw std::runtime_error("BIN_UNKNOWN found");

      case BIN_ADD:     return { .ictKind = ict::OP_ADD, .hasPointerSizeAdjust = true };
      case BIN_SUB:     return { .ictKind = ict::OP_SUB, .hasPointerSizeAdjust = true };
      case BIN_MUL:     return { .ictKind = ict::OP_MUL, };
      case BIN_DIV:     return { .ictKind = ict::OP_DIV, };
      case BIN_MOD:     return { .ictKind = ict::OP_MOD, };
      case BIN_RSH:     return { .ictKind = ict::OP_RSH, };
      case BIN_LSH:     return { .ictKind = ict::OP_LSH, };
      case BIN_BOR:     return { .ictKind = ict::OP_OR,  };
      case BIN_BAND:    return { .ictKind = ict::OP_AND, };
      case BIN_BXOR:    return { .ictKind = ict::OP_XOR, };
      case BIN_LT:      return { .ictKind = ict::OP_LT,  };
      case BIN_LE:      return { .ictKind = ict::OP_LE,  };
      case BIN_GT:      return { .ictKind = ict::OP_GT,  };
      case BIN_GE:      return { .ictKind = ict::OP_GE,  };
      case BIN_EQ:      return { .ictKind = ict::OP_EQ,  };
      case BIN_NEQ:     return { .ictKind = ict::OP_NEQ, };
      case BIN_OR:      return { .ictKind = ict::OP_OR,  .isShortCircuiting = true };
      case BIN_AND:     return { .ictKind = ict::OP_AND, .isShortCircuiting = true };

      case BIN_IADD:    return { .ictKind = ict::OP_ADD, .isInPlace = true, .hasPointerSizeAdjust = true };
      case BIN_ISUB:    return { .ictKind = ict::OP_SUB, .isInPlace = true, .hasPointerSizeAdjust = true };
      case BIN_IMUL:    return { .ictKind = ict::OP_MUL, .isInPlace = true };
      case BIN_IDIV:    return { .ictKind = ict::OP_DIV, .isInPlace = true };
      case BIN_IMOD:    return { .ictKind = ict::OP_MOD, .isInPlace = true };
      case BIN_IRSH:    return { .ictKind = ict::OP_RSH, .isInPlace = true };
      case BIN_ILSH:    return { .ictKind = ict::OP_LSH, .isInPlace = true };
      case BIN_IBXOR:    return { .ictKind = ict::OP_XOR, .isInPlace = true };
      case BIN_IBAND:    return { .ictKind = ict::OP_AND, .isInPlace = true };
      case BIN_IBOR:     return { .ictKind = ict::OP_OR,  .isInPlace = true };

      case BIN_COMMA:   return { .ictKind = ict::OP_UNKNOWN_OP, .isSpecial = true };
      case BIN_ASSIGN:  return { .ictKind = ict::OP_UNKNOWN_OP, .isInPlace = true,  .isSpecial = true };
      case BIN_SPACE:   return { .ictKind = ict::OP_UNKNOWN_OP, .isSpecial = true };
  }
}

static IrExprResult l_irBinary(IRGenCtx *ctx, Binary *binary, bool valueNeeded) {
    auto info = l_binaryInfo(binary->kind());
    
    if (info.isShortCircuiting)
        return l_irShortCircuitingBinary(ctx, binary);

    if (binary->kind() == BIN_COMMA) {
        auto left = irExpr(ctx, binary->left().get(), false),
            right = irExpr(ctx, binary->right().get(), valueNeeded);
        return right;
    }

    if (binary->kind() == BIN_ASSIGN) {
        auto left = irExpr(ctx, binary->left().get(), false),
            right = irExpr(ctx, binary->right().get());
        if (!left.pointer)
            throw misc::SourceError(binary->token(), "Cannot assign to non-lvalue");
        auto type = binary->left()->type()->toICT();
        if (!type) // TODO: non-primitive via memcpy
            throw misc::SourceError(binary->token(), "Assignment operators work only with primitive types");
        ctx->emit(ict::OP_STORE, std::move(type), left.pointer, right.value);
        return IrExprResult(right.value, left.pointer);
    }

    if (binary->kind() == BIN_SPACE)
        return l_irCall(ctx, binary);

    if (info.isSpecial)
        throw misc::SourceError(binary->token(), "compiler error: Special operation not handled by it's if in irgen");

    if (!valueNeeded && !info.isInPlace)
        return nullptr;

    auto left = irExpr(ctx, binary->left().get()),
        right = irExpr(ctx, binary->right().get());

    if (info.hasPointerSizeAdjust) { // pointer math
        if (binary->left()->type()->isPointer()) {
            size_t size = binary->left()->type()->unwrapNamed()->as<PointerType>()->to()->byteSize();
            if (size == 0) size = 1;
            right.value = ctx->emit(
                    ict::OP_MUL, nullptr, right.value,
                    size
            );
        } else if (binary->right()->type()->isPointer()) {
            if (info.isInPlace)
                throw misc::SourceError(binary->token(), "Inplace add of pointer to number");
            size_t size = binary->right()->type()->unwrapNamed()->as<PointerType>()->to()->byteSize();
            if (size == 0) size = 1;
            left.value = ctx->emit(
                    ict::OP_MUL, nullptr, left.value,
                    size
            );
        }
    }

    if (!info.isInPlace) { // a + b
        return ctx->emit(info.ictKind, nullptr, left.value, right.value);
    } else { // a += b
        if (!left.pointer)
            throw misc::SourceError(binary->token(), "Inplace operator requires lvalue");
        auto type = binary->left()->type()->toICT();
        if (!type)
            throw misc::SourceError(binary->token(), "Inplace operators work only with primitive types");
        auto value = ctx->emit(info.ictKind, nullptr, left.value, right.value);
        ctx->emit(ict::OP_STORE, std::move(type), left.pointer, value);
        return IrExprResult(value, left.pointer);
    }
}

IrExprResult irExpr(IRGenCtx *ctx, Expr *expr, bool valueNeeded) {
    if (auto num = dynamic_cast<Number*>(expr)) {
        if (!valueNeeded) return nullptr;
        return ctx->emit(ict::OP_CONST, nullptr, num->val());
    }

    if (auto str = dynamic_cast<String*>(expr)) {
        if (!valueNeeded) return nullptr;
        auto blob = ctx->module()->createString(str->val());
        return ctx->emit(ict::OP_GLOBALPTR, nullptr, blob);
    }

    if (auto unary = dynamic_cast<Unary*>(expr))
        return l_irUnary(ctx, unary, valueNeeded);

    if (auto binary = dynamic_cast<Binary*>(expr))
        return l_irBinary(ctx, binary, valueNeeded);
    
    if (auto name = dynamic_cast<Name*>(expr)) {
        assert(name->decl() != nullptr);
        auto addr = name->decl()->genAddr(ctx->curBlock);
        assert(addr != nullptr);

        auto type = name->type()->toICT();
        if (valueNeeded && !type)
            throw misc::SourceError(name->token(), "Cannot load non-primitive types");

        return IrExprResult(
            valueNeeded ? ctx->emit(ict::OP_LOAD, std::move(type), addr) : nullptr,
            addr
        );
    }

    throw std::runtime_error("Unknown expression type!");
}

void l_irIfElse(IRGenCtx *ctx, IfElse *ifElse) {
    auto cond = irExpr(ctx, ifElse->cond().get());
    auto then = ctx->newBlockAfter(ctx->curBlock),
         followup = ctx->newBlockAfter(then);

    then->addTag("if.then");
    followup->addTag("if.followup");
    if (ifElse->otherwise()) {
        //
        // cur -- then --- followup
        //   \_ otherwise _/
        //
        auto otherwise = ctx->newBlockAfter(then);
        otherwise->addTag("if.else");
        ctx->emit(ict::OP_BC, nullptr, cond.value, then, otherwise);

        ctx->curBlock = then;
        irStmnt(ctx, ifElse->then().get()); // curBlock may change
        if (!ctx->blockIsEnded())
            ctx->emit(ict::OP_BR, nullptr, followup);

        ctx->curBlock = otherwise;
        irStmnt(ctx, ifElse->otherwise().get()); // curBlock may change
        if (!ctx->blockIsEnded())
            ctx->emit(ict::OP_BR, nullptr, followup);

        // TODO: we may have dead code if two branches returned, do something about that

        ctx->curBlock = followup;
    } else {
        //
        // cur -- then -- followup
        //   \____________/
        //
        ctx->emit(ict::OP_BC, nullptr, cond.value, then, followup);

        ctx->curBlock = then;
        irStmnt(ctx, ifElse->then().get()); // curBlock may change
        if (!ctx->blockIsEnded())
            ctx->emit(ict::OP_BR, nullptr, followup);
        ctx->curBlock = followup;
    }
}

static void l_irWhile(IRGenCtx *ctx, While *whl) {
    auto check = ctx->newBlockAfter(ctx->curBlock);
    auto body = ctx->newBlockAfter(check);
    auto followup = ctx->newBlockAfter(body);

    check->addTag("while.check");
    body->addTag("while.body");
    followup->addTag("while.followup");

    ctx->emit(ict::OP_BR, nullptr, check);

    ctx->curBlock = check;
    auto cond = irExpr(ctx, whl->cond().get());
    ctx->emit(ict::OP_BC, nullptr, cond.value, body, followup);

    ctx->curBlock = body;
    irStmnt(ctx, whl->body().get());
    if (!ctx->blockIsEnded())
        ctx->emit(ict::OP_BR, nullptr, check);

    ctx->curBlock = followup;
}

void irStmnt(IRGenCtx *ctx, Statement *stmnt) {
    if (auto block = dynamic_cast<Block*>(stmnt)) {
        for (auto i : block->items())
            irStmnt(ctx, i);
        return;
    }

    if (auto expr = dynamic_cast<ExprInBlock*>(stmnt)) {
        irExpr(ctx, expr->expr().get());
        return;
    }

    if (auto decl = dynamic_cast<VarDeclStatement*>(stmnt)) {
        for (auto i : decl->decls()) {
            auto alloc = ctx->curBlock->operations().createEnd(ict::OP_ALLOCA, i->type()->toICT());
            alloc->addTag(ict::Tag(std::string("var.") + std::string(i->name())));
            i->setAddr(alloc);
            if (i->initExpr()) {
                auto val = irExpr(ctx, i->initExpr().get());
                ctx->emit(ict::OP_STORE, i->type()->toICT(), alloc, val.value);
            }
        }
        return;
    }

    if (auto ifElse = dynamic_cast<IfElse*>(stmnt))
        return l_irIfElse(ctx, ifElse);
    
    if (auto ret = dynamic_cast<ReturnStatement*>(stmnt)) {
        if (ret->expr()) {
            auto res = irExpr(ctx, ret->expr().get());
            ctx->emit(ict::OP_RET, nullptr, res.value);
        } else {
            ctx->emit(ict::OP_RET, nullptr);
        }
        return;
    }

    if (auto whl = dynamic_cast<While*>(stmnt))
        return l_irWhile(ctx, whl);
    
    throw std::runtime_error("Unknown statement type!");
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
    scl::irStmnt(&ctx, func->body().get());

    if (func->returnType()->isVoid()) {
        ctx.curBlock->operations().createEnd(ict::OP_RET, nullptr);
    } else if (!ctx.blockIsEnded()) {
        auto decl = ctx.module()->findOrDeclareFunc("_ict.sanitize.returnMissing", ict::Type::void_t());
        auto fname = ctx.module()->createString(func->name());
        ctx.emit(ict::OP_CALL, nullptr, decl, fname);
        ctx.emit(ict::OP_RET, nullptr, decl);
    }
}

ict::Integer evalConst(Expr *expr) {
    // TODO: expressions, other constants
    if (auto num = dynamic_cast<Number*>(expr)) {
        return num->val();
    } else if (auto unary = dynamic_cast<Unary*>(expr)) {
        auto val = evalConst(unary->val().get());
        switch (unary->kind()) {
            case UN_NEG: return -val;
            default:
                throw misc::SourceError(unary->token(), "Only negation is supported in consts");
        }
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
