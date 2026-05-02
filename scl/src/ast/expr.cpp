#include "scl/ast/expr.hpp"
#include "misclib/dump_stream.hpp"

using namespace misc::color;

namespace scl {

ict::View binKind2str(BinaryKind k) {
    #define GEN(k) case BIN_##k: return #k;
    switch(k) {
        GEN(UNKNOWN);
        GEN(ADD);
        GEN(SUB);
        GEN(MUL);
        GEN(DIV);
        GEN(MOD);
        GEN(LT);
        GEN(LE);
        GEN(GT);
        GEN(GE);
        GEN(EQ);
        GEN(NEQ);
        GEN(SPACE);
        GEN(COMMA);
        default: return "(unknown)";
    }
#undef GEN
}

ict::View unKind2str(UnaryKind k) {
#define GEN(k) case UN_##k: return #k;
    switch(k) {
        GEN(UNKNOWN);
        GEN(NEG);
        GEN(REF);
        GEN(DEREF);
        default: return "(unknown)";
    }
#undef GEN
}

void Binary::dump(std::ostream &os) const {
    os << BLUE << "Binary" << PURPLE << " " << binKind2str(kind()) << RST << "\n" 
        << misc::beginBlock<< DGRAY << "l: " << RST << *left()
        << DGRAY << "r: " << RST << *right()
        << misc::endBlock;
}

void Unary::dump(std::ostream &os) const {
    os << BLUE << "Unary" << PURPLE << " " << unKind2str(kind()) << RST << "\n"
        << misc::beginBlock << *val() << misc::endBlock;
}

void Number::dump(std::ostream &os) const {
    os << BLUE << "Number" << YELLOW << " " << val() << RST << "\n";
}

void Name::dump(std::ostream &os) const {
    os << BLUE << "Name" << RST << " " << name() << DGRAY;
    if (scopeItem())
        os << DGRAY << " (resolved to " << GREEN << "ScopeItem " << YELLOW << scopeItem() << DGRAY << ")\n" << RST;
    else
        os << DGRAY << " (not resolved yet)\n" << RST;
}

void ArgPack::dump(std::ostream &os) const {
    os << BLUE << "ArgPack" << RST << "\n" << misc::beginBlock;
    size_t idx = 0;
    for (auto i : items()) {
        os << DGRAY << (idx++) << ": " << RST << *i;
    }
    os << misc::endBlock;
}

};
