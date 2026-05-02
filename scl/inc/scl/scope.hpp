/**
 * Variable visibility scopes.
 * Initial AST has no scopes, they are added to it later.
 */
#pragma once
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "misclib/tree.hpp"

namespace scl {

class Scope;

class ScopeItem : public misc::Item<ScopeItem, misc::SlotVector<Scope, ScopeItem>> {
private:
    std::string m_name;
    ict::Operation *m_op = nullptr;

public:
    ScopeItem(misc::View name, ict::Operation *op = nullptr) :m_name(name), m_op(op) {}

    void setAddr(ict::Operation *op) { m_op = op; }
    ict::Operation *addr() { return m_op; }
    const ict::Operation *addr() const { return m_op; }
    misc::View name() const { return m_name; }

    // TODO: type info
};

class Scope : public misc::Item<Scope, misc::SlotVector<Scope, Scope>> {
    misc::SlotVector<Scope, Scope> m_childScopes;
    misc::SlotVector<Scope, ScopeItem> m_items;
public:

    Scope() :m_childScopes(this), m_items(this) {}

    auto &childScopes() { return m_childScopes; }
    auto &childScopes() const { return m_childScopes; }
    auto &items() { return m_items; }
    auto &items() const { return m_items; }

    Scope *createSubscope() { return childScopes().createEnd(); }

    template<typename ...Args>
    ScopeItem *createItem(Args &&...args) {
        return items().createEnd(std::forward<Args>(args)...);
    }

    bool isGlobal() { return parent() == nullptr; }

    ScopeItem *resolve(misc::View name) {
        for (auto it : items())
            if (it->name() == name) return it;
        return parent() ? parent()->resolve(name) : nullptr;
    }
};

};
