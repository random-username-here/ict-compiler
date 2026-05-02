#include "scl/ast/block.hpp"
#include "misclib/dump_stream.hpp"
#include <ostream>

using namespace misc::color;

namespace scl {

void Block::dump(std::ostream &os) const {
    os << RED << "Block\n" << misc::beginBlock;
    for (auto i : items()) os << *i;
    os << misc::endBlock;
}

void ExprInBlock::dump(std::ostream &os) const {
    os << RED << "ExprInBlock\n" << misc::beginBlock << *expr() << misc::endBlock;
}

void VarDeclStatement::dump(std::ostream &os) const {
    os << RED << "VarDeclStatement " << RST << '\n' << misc::beginBlock;
    for (auto i : decls())
        os << *i;
    os << misc::endBlock;
}

void VarDecl::dump(std::ostream &os) const {
    os << RED << "VarDecl " << RST << name();
    if (scopeItem())
        os << DGRAY << " (in scope as " << GREEN << "ScopeItem " << YELLOW << scopeItem() << DGRAY << ")" << RST;
    else
        os << DGRAY << " (not attached to scope)" << RST;

    if (initExpr())
        os << "\n" << misc::beginBlock << *initExpr() << misc::endBlock;
    else
        os << DGRAY << " (uninitialized)\n";
}

void IfElse::dump(std::ostream &os) const {
    os << RED << "IfElse " << RST << "\n" << misc::beginBlock;
    os << DGRAY << "cond: " << RST << *cond();
    os << DGRAY << "then: " << RST << *then();
    if (otherwise())
        os << DGRAY << "else: " << RST << *otherwise();
    os << misc::endBlock;
}

};
