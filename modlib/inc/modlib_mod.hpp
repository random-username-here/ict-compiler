#pragma once

#include <string_view>
class ModManager;

class Mod {
public:
    virtual std::string_view id() const = 0;
    virtual std::string_view brief() const = 0;

    virtual void on_resolveDeps(ModManager *mm) const {}
    virtual void on_depsResolved(ModManager *mm) const {}
    virtual void on_beforeCleanup(ModManager *mm) const {}

    virtual ~Mod() {}
};
