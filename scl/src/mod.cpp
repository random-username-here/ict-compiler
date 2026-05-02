#include "ict/mod.hpp"
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
            auto decl = into->findOrDeclareFunc("expr_demo", ict::Type::void_t());
            auto impl = decl->implement();
            auto first = impl->createBlock("start");
            scl::IRGenCtx ctx = { .curBlock = first };

            auto ast = scl::parseStatement(source);
            misc::info(TAG) << "Parsed expr:\n" << misc::beginBlock << *ast << misc::endBlock;
            scl::Scope top;
            scl::resolveScopes(ast.get(), &top);
            misc::info(TAG) << "Resolved scopes:\n" << misc::beginBlock << *ast << misc::endBlock;
            scl::irStmnt(&ctx, ast.get());
            misc::info(TAG) << "IR generated!";
            return true;
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
