#include "scl/ast/type.hpp"
#include "misclib/parse.hpp"
#include "scl/parsing.hpp"
namespace scl {

UPtr<Type> parseType(View &source) {
    auto tok = misc::tokenize(source, misc::TOKF_PTRS);
    if (tok.type == misc::TOK_DEREF) {
        return PointerType::create(parseType(source));
    } else if (tok.type == misc::TOK_NAME) {
        auto kind = PrimitiveType::kindFromString(tok.view);
        if (kind)
            return PrimitiveType::create(*kind);
        else
            throw misc::SourceError(tok, "Unknown type name");
    } else {
        throw misc::SourceError(tok, "Expected type here");
    }
}

};
