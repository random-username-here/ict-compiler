#include "misclib/parse.hpp"
#include "scl/ast/block.hpp"
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
            if (i->initExpr())
                resolveScopes(i->initExpr().get(), parent);
            i->setScopeItem(parent->createItem(i->name()));
        }
    } else if (auto ifelse = dynamic_cast<IfElse*>(stmnt)) {
        resolveScopes(ifelse->cond().get(), parent);
        resolveScopes(ifelse->then().get(), parent);
        resolveScopes(ifelse->otherwise().get(), parent);
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
        if (!st) {
            if (name->hasToken())
                throw misc::SourceError(name->token(), "Unknown symbol!");
            else
                throw std::runtime_error("Unknown name found, no token");
        }
        name->setScopeItem(st);
    } else if (auto num = dynamic_cast<Number*>(expr)) {
        // nothing
    } else {
        throw std::runtime_error("Unknown expr type");
    }
}

};
