#include "misclib/parse.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/module.hpp"
#include "scl/ast/type.hpp"
#include "scl/passes.hpp"
#include "misclib/dump_stream.hpp"
#include <sstream>
#define TAG "scl.types"

namespace scl {

// TODO: check const qualifiers

static void l_checkAndAddCast(Expr *expr, Type *to, misc::Token ref) {
    if (expr->type()->isVoid())
        goto bad_cvt;
    // ptr <-> ints <-> bool are good
    if (to->isBool() && !expr->type()->isBool()) {
        auto slot = expr->slot();
        auto cmp = Binary::create(BIN_NEQ, slot->replace(nullptr), Number::create(0));
        resolveTypes(cmp.get());
        slot->replace(std::move(cmp));
    }
    return;
bad_cvt:
    std::ostringstream s;
    s << "Cannot convert value of type " << *expr->type() << " to " << *to;
    throw misc::SourceError(ref, s.str());
}

static UPtr<Type> l_binaryResult(Binary *bin) {

    auto leftType = bin->left()->type().get()->unwrapNamed(),
         rightType = bin->right()->type().get()->unwrapNamed();

    if ((bin->kind() != BIN_SPACE && bin->kind() != BIN_COMMA) && (!leftType->isComplete() || !rightType->isComplete()))
        goto bad_op;

    switch (bin->kind()) {

        case BIN_UNKNOWN: assert(false);

        case BIN_ADD: case BIN_SUB:
            if (rightType->isPointer()) {
                if (!leftType->isInteger())
                    goto bad_op;
                return rightType->copy();
            }
            // fallthrough

        case BIN_IADD: case BIN_ISUB:
            // unlike not in-place, we cannot do int += pointer
            if (leftType->isPointer()) {
                if (!rightType->isInteger())
                    goto bad_op;
                return leftType->copy();
            }
            // else fallthrough

        case BIN_MUL: case BIN_DIV: case BIN_MOD:
        case BIN_IMUL: case BIN_IDIV: case BIN_IMOD:
        case BIN_LSH: case BIN_RSH:
        case BIN_ILSH: case BIN_IRSH:
        case BIN_BOR: case BIN_BAND: case BIN_BXOR:
        case BIN_IBOR: case BIN_IBAND: case BIN_IBXOR:
        space_converted_to_mul:
            // only integers allowed
            if (!leftType->isInteger() || !rightType->isInteger())
                goto bad_op;
            return leftType->byteSize() < rightType->byteSize() 
                ? rightType->copy() : leftType->copy();

        case BIN_LT: case BIN_LE: case BIN_GT:
        case BIN_GE: case BIN_EQ: case BIN_NEQ:
            // comparsions, ints or pointers
            if (!(leftType->isPointer() || leftType->isInteger()))
                goto bad_op;
            if (!(rightType->isPointer() || rightType->isInteger()))
                goto bad_op;
            return PrimitiveType::create(PrimitiveType::BOOL);

        case BIN_OR: case BIN_AND: {
            // must be booleans (or cast to booleans)
            auto boolType = PrimitiveType::create(PrimitiveType::BOOL);
            l_checkAndAddCast(bin->left().get(), boolType.get(), bin->left()->token());
            l_checkAndAddCast(bin->right().get(), boolType.get(), bin->right()->token());
            return boolType;
        }

        case BIN_COMMA:
            return rightType->copy();

        case BIN_ASSIGN:
            // TODO: check lvalue here instead of in irgen
            return leftType->copy();
       
        case BIN_SPACE:
            if (leftType->isInteger()) {
                // something like `3 a`
                bin->setKind(BIN_MUL);
                goto space_converted_to_mul;
            }

            if (leftType->isFunction()) { // convert foo(...) to (&foo)(...)
                bin->left() = Unary::create(bin->token(), UN_REF, bin->left()->replace(nullptr));
                resolveTypes(bin->left().get());
                leftType = bin->left()->type().get()->unwrapNamed();
            }

            if (!rightType->isPack()) { // convert sin 3 to sin(3)
                bin->right() = ArgPack::create(bin->token(), bin->right()->replace(nullptr));    
                resolveTypes(bin->right().get());
                rightType = bin->right()->type().get()->unwrapNamed();
            }

            if (!leftType->isFunctionPtr()) 
                throw misc::SourceError(bin->token(), "Cannot call not a function pointer");

            auto funcType = leftType->as<PointerType>()->to()->as<FunctionType>();
            auto pack = static_cast<ArgPack*>(bin->right().get());
            if (pack->items().size() != funcType->args().size())
                throw misc::SourceError(bin->token(), "Invalid argument count");
            for (size_t i = 0; i < pack->items().size(); ++i)
                l_checkAndAddCast(pack->items()[i], funcType->args()[i], pack->items()[i]->token());
            return funcType->returnType()->copy();
    }

bad_op:
    std::ostringstream s;
    s << "Cannot perform `" << *bin->left()->type() << " ";
    if (bin->token().view.data() != nullptr)
        s << bin->token().view;
    else
        s << binKind2str(bin->kind());
    s << " " << *bin->right()->type() << "`";
    throw misc::SourceError(bin->token(), s.str());
}

static UPtr<Type> l_unaryResult(Unary *un) {
    auto valType = un->val()->type().get();
    if (!valType->isComplete() && un->kind() != UN_REF)
        goto bad_op;

    switch (un->kind()) {
        case UN_UNKNOWN: 
            assert(false);

        case UN_NOT: {
            auto boolType = PrimitiveType::create(PrimitiveType::BOOL);
            l_checkAndAddCast(un->val().get(), boolType.get(), un->val()->token());
            return boolType;
        }

        case UN_NEG: case UN_INV:
        case UN_INC_PRE: case UN_INC_POST:
        case UN_DEC_PRE: case UN_DEC_POST:
            // TODO: check lvalue here instead of in irgen
            if (!valType->isInteger() && !valType->isPointer())
                goto bad_op;
            return valType->copy();

        case UN_REF:
            // TODO: same
            return PointerType::create(valType->copy());

        case UN_DEREF:
            if (auto ptr = dynamic_cast<PointerType*>(un->val()->type().get()))
                return ptr->to()->copy();
            else
                goto bad_op;
    }

bad_op:
    std::ostringstream s;
    s << "Cannot perform `" << un->token().view << " " << *un->val()->type() << "`";
    throw misc::SourceError(un->token(), s.str());
}

void resolveTypes(Expr *expr) {
    if (expr->type()) return;
    if (auto bin = dynamic_cast<Binary*>(expr)) {
        resolveTypes(bin->left().get());
        resolveTypes(bin->right().get());
        bin->type() = l_binaryResult(bin);
    } else if (auto un = dynamic_cast<Unary*>(expr)) {
        resolveTypes(un->val().get());
        un->type() = l_unaryResult(un);
    } else if (auto name = dynamic_cast<Name*>(expr)) {
        // this can return incomplete type!
        misc::verb(TAG) << "... resolve " << name->decl()->name() << " -> " << *name->decl()->type();
        name->type() = name->decl()->type()->copy();
    } else if (auto num = dynamic_cast<Number*>(expr)) {
        num->type() = PrimitiveType::create(PrimitiveType::INT64);
    } else if (auto str = dynamic_cast<String*>(expr)) {
        auto chartype = PrimitiveType::create(PrimitiveType::INT8);
        chartype->setConst(true);
        str->type() = PointerType::create(std::move(chartype));
    } else if (auto pack = dynamic_cast<ArgPack*>(expr)) {
        for (auto i : pack->items())
            resolveTypes(i);
        pack->type() = PackType::create(); // TODO: tuples
    } else {
        throw std::runtime_error("Unknown expr type");
    }
}

void resolveTypes(Statement *stmnt, Function *func) {
    if (auto blk = dynamic_cast<Block*>(stmnt)) {
        for (auto i : blk->items())
            resolveTypes(i, func);
    } else if (auto expr = dynamic_cast<ExprInBlock*>(stmnt)) {
        resolveTypes(expr->expr().get());
    } else if (auto decl = dynamic_cast<VarDeclStatement*>(stmnt)) {
        for (auto i : decl->decls()) {
            if (i->initExpr())
                resolveTypes(i->initExpr().get());
            if (i->type())
                resolveTypes(i->type().get());
            if (!i->initExpr() && !i->type())
                throw misc::SourceError(i->token(), "No type specified and no initializer expression to deduce type from");
            else if (!i->type())
                i->type() = i->initExpr()->type()->copy();
            else if (i->type() && i->initExpr())
                l_checkAndAddCast(i->initExpr().get(), i->type().get(), i->token());
            if (!i->type()->isComplete()) {
                std::stringstream s;
                s << "Cannot declare variable of incomplete type " << *i->type();
                throw misc::SourceError(i->token(), s.str());
            }
        }
    } else if (auto ifelse = dynamic_cast<IfElse*>(stmnt)) {
        auto b = PrimitiveType(PrimitiveType::BOOL);
        resolveTypes(ifelse->cond().get());
        l_checkAndAddCast(ifelse->cond().get(), &b, ifelse->token());
        resolveTypes(ifelse->then().get(), func);
        if (ifelse->otherwise())
            resolveTypes(ifelse->otherwise().get(), func);
    } else if (auto ret = dynamic_cast<ReturnStatement*>(stmnt)) {
        if (func->body().get() == stmnt && func->hasImplcitReturnType() && ret->expr()) {
            // deduce return type
            resolveTypes(ret->expr().get());
            return;
        }

        if (ret->expr() && func->returnType()->isVoid())
            throw misc::SourceError(ret->token(), "Cannot return value from void function");
        if (!ret->expr() && !func->returnType()->isVoid())
            throw misc::SourceError(ret->token(), "Must return something in non-void function");
        if (ret->expr()) {
            resolveTypes(ret->expr().get());
            l_checkAndAddCast(ret->expr().get(), func->returnType().get(), ret->token());
        }
    } else if (auto whl = dynamic_cast<While*>(stmnt)) {
        auto b = PrimitiveType(PrimitiveType::BOOL);
        resolveTypes(whl->cond().get());
        l_checkAndAddCast(whl->cond().get(), &b, whl->token());
        resolveTypes(whl->body().get(), func);
    } else {
        throw std::runtime_error("Unknown statement type");
    }
}


void resolveTypes(Module *mod) {
    for (auto i : mod->entries()) {
        if (auto func = dynamic_cast<Function*>(i)) {
            auto bodyAsReturn = dynamic_cast<ReturnStatement*>(func->body().get());
            bool isFunctionExpr = bodyAsReturn && bodyAsReturn->expr();
            
            if (func->hasImplcitReturnType() && isFunctionExpr)
                func->type() = BadType::create();

            misc::verb(TAG) << "Resolving types for " << func->name();
            resolveTypes(func->returnType().get());
            misc::verb(TAG) << "... return type is " << *func->returnType().get();
            for (auto i : func->args()) {
                resolveTypes(i->type().get());
                misc::verb(TAG) << "... arg " << i->name() << " is " << *i->type();
            }
            if (func->body())
                resolveTypes(func->body().get(), func);

            if (func->hasImplcitReturnType() && isFunctionExpr) {
                func->returnType() = bodyAsReturn->expr()->type()->copy();
                misc::info(TAG) << "Type fixup for " << func->name();
            }
            func->genDeclType();
            misc::info(TAG) << "Resolved types for " << func->name() << " as " << *func->type();

        } else if (auto td = dynamic_cast<TypeDef*>(i)) {
            resolveTypes(td->aliasedType().get());
            misc::info(TAG) << "Resolved types for typedef " << td->name();
        } else if (auto gvb = dynamic_cast<GlobalVarBlock*>(i)) {
            for (auto v : gvb->vars()) {
                resolveTypes(v->type().get());
                if (v->initExpr())
                    resolveTypes(v->initExpr().get());
                if (!v->type()) {
                    if (v->initExpr())
                        v->type() = v->initExpr()->type()->copy();
                    else
                        throw misc::SourceError(v->token(), "No type specified and no initializer to infer type from");
                } else if (v->initExpr()) {
                    l_checkAndAddCast(v->initExpr().get(), v->type().get(), v->token());
                }
            }
        }
    }
}

void resolveTypes(Type *type) {
    if (auto named = dynamic_cast<NamedType*>(type)) {
        named->resolved() = named->decl()->aliasedType()->copy();
        misc::info(TAG) << "... Resolved type " << named->decl()->name() << " as " << *named->decl()->aliasedType();
    } else if (auto ptr = dynamic_cast<PointerType*>(type)) {
        resolveTypes(ptr->to().get());
    } else if (auto func = dynamic_cast<FunctionType*>(type)) {
        resolveTypes(func->returnType().get());
        for (auto i : func->args())
            resolveTypes(i);
    }
}

};
