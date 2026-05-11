#include "scl/ast/module.hpp"
#include "misclib/dump_stream.hpp"
#include "scl/ast/type.hpp"
#include <ostream>
using namespace misc::color;

namespace scl {

void Module::dump(std::ostream &os) const {
    os << BOLD << GREEN << "Module\n" << RST << misc::beginBlock;
    for (auto f : entries())
        os << *f;
    os << misc::endBlock;
}

void Function::dump(std::ostream &os) const {
    os << BOLD << GREEN << "Function" << RST << " " << name();
    if (!body())
        os << DGRAY << " (decl)" << RST;
    os << "\n" << misc::beginBlock;
    os << DGRAY << "returns: " << RST << *returnType() << '\n';
    for (auto arg : args())
        os << DGRAY << "arg: " << RST << *arg;
    if (body())
        os << DGRAY << "body: " << RST << *body();
    os << misc::endBlock;
}

void FunctionArg::dump(std::ostream &os) const {
    os << GREEN << "FunctionArg" << RST << " " << name();
    if (type()) os << " " << *type();
    else os << DGRAY << " (type not resolved)" << RST;
    os << "\n";
}
    
void Function::genDeclType() {
    auto ft = FunctionType::create();
    ft->returnType() = returnType()->copy();
    for (auto i : args())
        ft->args().push(i->type()->copy());
    type() = std::move(ft);
}

};
