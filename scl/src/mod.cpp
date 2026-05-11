#include "ict/mod.hpp"
#include "ict/ir.hpp"
#include "misclib/parse.hpp"
#include "modlib_mod.hpp"
#include "misclib/dump_stream.hpp"
#include "scl/parsing.hpp"
#include "scl/irgen.hpp"
#include "scl/passes.hpp"
#include "scl/scope.hpp"

#define TAG "scl.main"

using namespace ict;

class SclFrontend : 
        public ict::Frontend
{
public:
    View id() const override { return "scl.frontend"; }
    std::string_view brief() const override { return "ICT frontend for SCL language"; }
    View langName() const override { return "scl"; }

    bool takesFile(View name) const override {
        return misc::endsWith(name, ".scl");
    }

    bool compile(Manager *mgr, Module *into) const override {
        misc::info(TAG) << "Parsing " << misc::ACCENT << mgr->filename() << misc::RST << '\n';
        View source = mgr->source();
        try {
            auto mod = scl::Module::create();
            misc::info(TAG) << "Parsing module...";
            while (scl::parseTopLevel(source, mod.get()));
            misc::info(TAG) << "Parsed module:\n" << misc::beginBlock << *mod << misc::endBlock;
            misc::info(TAG) << "Resolving scopes...";
            scl::Scope top; // top scope, in which multiple modules can be added
            scl::resolveScopes(mod.get(), &top);
            misc::info(TAG) << "With scopes generated:\n" << misc::beginBlock << *mod << misc::endBlock;
            misc::info(TAG) << "Resolving types...";
            scl::resolveTypes(mod.get());
            misc::info(TAG) << "With types resolved:\n" << misc::beginBlock << *mod << misc::endBlock;
            misc::info(TAG) << "Generating IR...";
            irModule(into, mod.get());
            misc::info(TAG) << "Generated IR:\n" << misc::beginBlock << *into << misc::endBlock;
            return false;
        } catch (misc::SourceError &e) {
            auto msg = misc::error(TAG);
            e.writeFormatted(msg.stream(), mgr->filename(), mgr->source());
            return false;
        }
        misc::info(TAG) << "Verifying generated IR...";

        if (into->verify()) {
            misc::info(TAG) << "IR is good\n";
            return true;
        } else {
            misc::error(TAG) << "IR has problems, bailing out\n";
            return false;
        }
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new SclFrontend();
}
