/**
 * Variable visibility scopes.
 * Initial AST has no scopes, they are added to it later.
 */
#pragma once
#include "misclib/defs.hpp"
#include "misclib/tree.hpp"
#include "scl/ast/basic.hpp"

namespace scl {

class Scope : public misc::Item<Scope, misc::SlotVector<Scope, Scope>> {
    misc::SlotVector<Scope, Scope> m_childScopes;
    std::vector<Decl*> m_decls;
public:

    Scope() :m_childScopes(this) {}

    auto &childScopes() { return m_childScopes; }
    auto &childScopes() const { return m_childScopes; }
    auto &decls() { return m_decls; }
    auto &decls() const { return m_decls; }

    Scope *createSubscope() { return childScopes().createEnd(); }

    void addDecl(Decl *d) { m_decls.push_back(d); }

    bool isGlobal() { return parent() == nullptr; }

    Decl *resolve(misc::View name) {
        for (auto it : decls())
            if (it->name() == name) return it;
        return parent() ? parent()->resolve(name) : nullptr;
    }
};

};
