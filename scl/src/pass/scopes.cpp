#include "misclib/parse.hpp"
#include "scl/ast/basic.hpp"
#include "scl/ast/block.hpp"
#include "scl/ast/expr.hpp"
#include "scl/ast/module.hpp"
#include "scl/ast/type.hpp"
#include "scl/passes.hpp"
#include <stdexcept>

namespace scl {

void resolveScopes(Statement *stmnt, Scope *parent) {
    if (auto blk = dynamic_cast<Block*>(stmnt)) {
        auto subscope = parent->createSubscope();
        for (auto i : blk->items())
            resolveScopes(i, subscope);
    } else if (auto expr = dynamic_cast<ExprInBlock*>(stmnt)) {
        resolveScopes(expr->expr().get(), parent);
    } else if (auto decl = dynamic_cast<VarDeclStatement*>(stmnt)) {
        for (auto i : decl->decls()) {
            if (i->type())
                resolveScopes(i->type().get(), parent);
            if (i->initExpr())
                resolveScopes(i->initExpr().get(), parent);
            parent->addDecl(i);
        }
    } else if (auto ifelse = dynamic_cast<IfElse*>(stmnt)) {
        resolveScopes(ifelse->cond().get(), parent);
        resolveScopes(ifelse->then().get(), parent);
        if (ifelse->otherwise())
            resolveScopes(ifelse->otherwise().get(), parent);
    } else if (auto ret = dynamic_cast<ReturnStatement*>(stmnt)) {
        if (ret->expr())
            resolveScopes(ret->expr().get(), parent);
    } else if (auto whl = dynamic_cast<While*>(stmnt)) {
        resolveScopes(whl->cond().get(), parent);
        resolveScopes(whl->body().get(), parent);
    } else {
        throw std::runtime_error("Unknown statement type");
    }
}

void resolveScopes(Expr *expr, Scope *parent) {
    if (auto bin = dynamic_cast<Binary*>(expr)) {
        resolveScopes(bin->left().get(), parent);
        resolveScopes(bin->right().get(), parent);
    } else if (auto un = dynamic_cast<Unary*>(expr)) {
        resolveScopes(un->val().get(), parent);
    } else if (auto name = dynamic_cast<Name*>(expr)) {
        auto st = parent->resolve(name->name());
        if (!st)
            throw misc::SourceError(name->token(), "Unknown symbol!");
        name->setDecl(st);
    } else if (auto num = dynamic_cast<Number*>(expr)) {
        // nothing
    } else if (auto str = dynamic_cast<String*>(expr)) {
        // also nothing
    } else if (auto pack = dynamic_cast<ArgPack*>(expr)) {
        for (auto i : pack->items())
            resolveScopes(i, parent);
    } else {
        throw std::runtime_error("Unknown expr type");
    }
}

void resolveScopes(Type *type, Scope *parent) {
    if (auto named = dynamic_cast<NamedType*>(type)) {
        auto decl = parent->resolve(named->name());
        if (!decl)
            throw misc::SourceError(named->token(), "Unknown type name!");
        auto tdecl = dynamic_cast<TypeDecl*>(decl);
        if (!decl)
            throw misc::SourceError(named->token(), "Not a type!");
        named->setDecl(tdecl);
    } else if (auto ptr = dynamic_cast<PointerType*>(type)) {
        resolveScopes(ptr->to().get(), parent);
    } else if (auto func = dynamic_cast<FunctionType*>(type)) {
        resolveScopes(func->returnType().get(), parent);
        for (auto i : func->args())
            resolveScopes(i, parent);
    }
}

void resolveScopes(Module *mod, Scope *parent) {
    for (auto i : mod->entries()) {
        if (auto func = dynamic_cast<Function*>(i)) {
            parent->addDecl(func);
            resolveScopes(func->returnType().get(), parent);
            for (auto i : func->args())
                resolveScopes(i->type().get(), parent);
            if (func->body()) {
                auto inner = parent->createSubscope();
                for (auto i : func->args())
                    inner->addDecl(i);
                resolveScopes(func->body().get(), inner);
            }
        } else if (auto td = dynamic_cast<TypeDef*>(i)) {
            parent->addDecl(td);
            resolveScopes(td->aliasedType().get(), parent);
        } else if (auto vdb = dynamic_cast<GlobalVarBlock*>(i)) {
            for (auto i : vdb->vars()) {
                parent->addDecl(i);
                resolveScopes(i->type().get(), parent);
                if (i->initExpr())
                    resolveScopes(i->initExpr().get(), parent);
            }
        }
    }
}

};
