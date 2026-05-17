#include "scl/ast/type.hpp"
#include "misclib/parse.hpp"
#include "scl/parsing.hpp"
namespace scl {

UPtr<Type> parseType(View &source) {
    auto tok = misc::tokenize(source, misc::TOKF_PTRS);
    if (tok.type == misc::TOK_DEREF) {
        return PointerType::create(parseType(source));
    } else if (tok.type == misc::TOK_NAME) {
        if (tok.view == "const") {
            auto t = parseType(source);
            t->setConst(true);
            return t;
        }
        auto kind = PrimitiveType::kindFromString(tok.view);
        if (kind)
            return PrimitiveType::create(*kind);
        else
            return NamedType::create(tok);
    } else {
        throw misc::SourceError(tok, "Expected type here");
    }
}

};
