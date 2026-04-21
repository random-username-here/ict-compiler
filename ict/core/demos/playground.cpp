/**
 * Playground for constructing IR manually (like frontend would do).
 */
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
#include "misclib/dump_stream.hpp"

using misc::UPtr;
using namespace misc::color;
#define TAG "playground"

int main() {
    misc::info(TAG) << "Creating a module manually";

    UPtr<ict::Module> mod = ict::Module::create("playground");
    
    auto debugPrintInt = mod->findOrDeclareFunc(
            "print_int", ict::Type::void_t(), ict::ArgDecl::create(ict::Type::i64_t(), "value")
    );

    auto main = mod->findOrDeclareFunc("main", ict::Type::void_t());
    auto impl = main->implement();
    auto builder = impl->createBlock("start")->operations().backInserter();
    auto num = builder.create(ict::OP_CONST, ict::Type::i64_t(), 42);
    builder.create(ict::OP_CALL, nullptr, debugPrintInt, num);
    builder.create(ict::OP_RET, nullptr);

    misc::info(TAG) << "Verifying";
    if (mod->verify()) {
        misc::info(TAG) << GREEN << "IR is valid";
    } else {
        misc::error(TAG) << RED << "IR has problems";
    }

    misc::info(TAG) << *mod;
}
