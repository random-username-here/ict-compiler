#include "misclib/parse.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/type.hpp"
#include "scl/passes.hpp"
#include "misclib/dump_stream.hpp"
#include <sstream>
namespace scl {

static void l_checkAndAddCast(Expr *expr, Type *to, misc::Token ref) {
    if (expr->type()->isVoid())
        goto bad_cvt;
    // ptr <-> i64 <-> bool are good
    // TODO: add cast
    return;
bad_cvt:
    std::ostringstream s;
    s << "Cannot convert value of type " << *expr->type() << " to " << *to;
    throw misc::SourceError(ref, s.str());
}

static UPtr<Type> l_binaryResult(Binary *bin) {
    if (bin->kind() != BIN_SPACE && (!bin->left()->type()->isComplete() || !bin->right()->type()->isComplete()))
        goto bad_op;
    switch (bin->kind()) {
        case BIN_UNKNOWN: assert(false);
        case BIN_ADD: case BIN_SUB:
            if (bin->left()->type()->isPointer()) {
                if (!bin->right()->type()->isInteger())
                    goto bad_op;
                return bin->left()->type()->copy();
            }
            if (bin->right()->type()->isPointer()) {
                if (!bin->left()->type()->isInteger())
                    goto bad_op;
                return bin->right()->type()->copy();
            }
            // else fallthrough
        case BIN_MUL:
        case BIN_DIV: case BIN_MOD:
        space_converted_to_mul:
            if (!bin->left()->type()->isInteger() || !bin->right()->type()->isInteger())
                goto bad_op;
            // TODO: chose smaller of those two
            return PrimitiveType::create(PrimitiveType::INT64);
        case BIN_LT: case BIN_LE: case BIN_GT:
        case BIN_GE: case BIN_EQ: case BIN_NEQ:
            if (!bin->left()->type()->isInteger() || !bin->right()->type()->isInteger())
                goto bad_op;
            return PrimitiveType::create(PrimitiveType::BOOL);
        case BIN_COMMA:
            return bin->right()->type()->copy();
        case BIN_ASSIGN:
            // we can assign to names or something unreffed
            if (auto unary = dynamic_cast<Unary*>(bin->left().get())) {
                if (unary->kind() == UN_DEREF) 
                    return bin->left()->type()->copy();
            } else if (auto name = dynamic_cast<Name*>(bin->left().get())) {
                // TODO: check if that is non-constant thing
                l_checkAndAddCast(bin->right().get(), name->decl()->type().get(), bin->token());
                return name->decl()->type()->copy();
            }
            throw misc::SourceError(bin->token(), "Can only assign to variables or deref-expressions");
        case BIN_SPACE:
            if (bin->right()->type()->isInteger() || bin->left()->type()->isInteger()) {
                bin->setKind(BIN_MUL);
                goto space_converted_to_mul;
            }
            if (bin->left()->type()->isFunction()) { // convert foo(...) to (&foo)(...)
                bin->left() = Unary::create(bin->token(), UN_REF, bin->left()->replace(nullptr));
                resolveTypes(bin->left().get());
            }
            if (!bin->right()->type()->isPack()) { // convert sin 3 to sin(3)
                bin->right() = ArgPack::create(bin->token(), bin->right()->replace(nullptr));
                resolveTypes(bin->right().get());
            }
            if (!bin->left()->type()->isFunctionPtr()) 
                throw misc::SourceError(bin->token(), "Cannot call not a function pointer");
            auto ft = static_cast<FunctionType*>(static_cast<PointerType*>(bin->left()->type().get())->to().get());
            auto pack = static_cast<ArgPack*>(bin->right().get());
            if (pack->items().size() != ft->args().size())
                throw misc::SourceError(bin->token(), "Invalid argument count");
            for (size_t i = 0; i < pack->items().size(); ++i)
                l_checkAndAddCast(pack->items()[i], ft->args()[i], pack->items()[i]->token());
            return ft->returnType()->copy();
    }

bad_op:
    std::ostringstream s;
    s << "Cannot perform `" << *bin->left()->type() << " " << bin->token().view << " " << *bin->right()->type() << "`";
    throw misc::SourceError(bin->token(), s.str());
}

static UPtr<Type> l_unaryResult(Unary *un) {
    if (!un->val()->type()->isComplete() && un->kind() != UN_REF)
        goto bad_op;
    switch (un->kind()) {
        case UN_UNKNOWN: assert(false);
        case UN_NEG:
            if (!un->val()->type()->isInteger())
                goto bad_op;
            return un->val()->type()->copy();
        case UN_REF:
            if (auto name = dynamic_cast<Name*>(un->val().get())) {
                return PointerType::create(name->type()->copy());
            } else {
                throw misc::SourceError(un->token(), "Cannot take reference of something which is not a name");
            }
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
        name->type() = name->decl()->type()->copy();
    } else if (auto num = dynamic_cast<Number*>(expr)) {
        num->type() = PrimitiveType::create(PrimitiveType::INT64);
    } else if (auto pack = dynamic_cast<ArgPack*>(expr)) {
        pack->type() = PackType::create(); // TODO: tuples
    } else {
        throw std::runtime_error("Unknown expr type");
    }
}

void resolveTypes(Statement *stmnt) {
    if (auto blk = dynamic_cast<Block*>(stmnt)) {
        for (auto i : blk->items())
            resolveTypes(i);
    } else if (auto expr = dynamic_cast<ExprInBlock*>(stmnt)) {
        resolveTypes(expr->expr().get());
    } else if (auto decl = dynamic_cast<VarDeclStatement*>(stmnt)) {
        for (auto i : decl->decls()) {
            if (i->initExpr())
                resolveTypes(i->initExpr().get());
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
        resolveTypes(ifelse->then().get());
        resolveTypes(ifelse->otherwise().get());
    } else {
        throw std::runtime_error("Unknown statement type");
    }
}

void resolveTypes(Module *mod) {
    for (auto i : mod->entries()) {
        if (auto func = dynamic_cast<Function*>(i)) {
            if (func->body())
                resolveTypes(func->body().get());
        }
    }
}

};
