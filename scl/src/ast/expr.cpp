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
        GEN(RSH);
        GEN(LSH);
        GEN(BOR);
        GEN(BAND);
        GEN(BXOR);

        GEN(LT);
        GEN(LE);
        GEN(GT);
        GEN(GE);
        GEN(EQ);
        GEN(NEQ);

        GEN(OR);
        GEN(AND);

        GEN(IADD);
        GEN(ISUB);
        GEN(IMUL);
        GEN(IDIV);
        GEN(IMOD);
        GEN(IRSH);
        GEN(ILSH);
        GEN(IBXOR);
        GEN(IBAND);
        GEN(IBOR);

        GEN(COMMA);
        GEN(ASSIGN);
        GEN(SPACE);

        default: return "(unknown)";
    }
#undef GEN
}

ict::View unKind2str(UnaryKind k) {
#define GEN(k) case UN_##k: return #k;
    switch(k) {
        GEN(UNKNOWN);
        GEN(NEG);
        GEN(NOT);
        GEN(INV);
        GEN(INC_PRE);
        GEN(INC_POST);
        GEN(DEC_PRE);
        GEN(DEC_POST);
        GEN(REF);
        GEN(DEREF);
        default: return "(unknown)";
    }
#undef GEN
}

void Binary::dump(std::ostream &os) const {
    os << BLUE << "Binary" << PURPLE << " " << binKind2str(kind()) << RST;
    if (type()) os << DGRAY << " -> " << RST << *type();
    os  << "\n" 
        << misc::beginBlock<< DGRAY << "l: " << RST << *left()
        << DGRAY << "r: " << RST << *right()
        << misc::endBlock;
}

void Unary::dump(std::ostream &os) const {
    os << BLUE << "Unary" << PURPLE << " " << unKind2str(kind()) << RST;
    if (type()) os << DGRAY << " -> " << RST << *type();
    os << "\n"
        << misc::beginBlock << *val() << misc::endBlock;
}

void Number::dump(std::ostream &os) const {
    os << BLUE << "Number" << YELLOW << " " << val() << RST << "\n";
}

void String::dump(std::ostream &os) const {
    os << BLUE << "String" << GREEN << " ";
    if (token().view.data() != nullptr) os << token().view;
    else os << "\"" << val() << "\""; // not escaped variant for some rare case when this is generated
    os << RST << "\n";
}

void Name::dump(std::ostream &os) const {
    os << BLUE << "Name" << RST << " " << name() << DGRAY;
    if (decl()) { // TODO: brief 
        os << DGRAY << " (resolved to " << RST << decl()->name();
        if (decl()->type())
            os << " " << *decl()->type();
        else
            os << DGRAY << " (type not known yet)";
        os << DGRAY << ")\n" << RST;
    } else
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
