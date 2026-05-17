#include "scl/ast/type.hpp"
#include "misclib/parse.hpp"
#include "scl/parsing.hpp"
namespace scl {

static UPtr<Type> l_parseFuncType(View &source) {
    auto opening = misc::tokenize(source, misc::TOKF_NONE);
    if (opening.type != '(')
        throw misc::SourceError(opening, "`(` expected");
    auto ft = FunctionType::create();

    misc::Token tok;
    auto v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    if (tok.type == ')') {
        source = v;
        goto args_end;
    }

    while (1) {
        ft->args().push(parseType(source));
        auto punct = misc::tokenize(source, misc::TOKF_NONE);
        if (punct.type == ',') continue;
        else if (punct.type == ')') break;
        else throw misc::SourceError(punct, "Expected `,` or `)` in arg list");
    }

args_end:
    ft->returnType() = parseType(source);
    return ft;
}

UPtr<Type> parseType(View &source) {
    auto tok = misc::tokenize(source, misc::TOKF_PTRS);
    if (tok.type == misc::TOK_DEREF) {
        return PointerType::create(parseType(source));
    } else if (tok.type == misc::TOK_NAME) {
        if (tok.view == "const") {
            auto t = parseType(source);
            t->setConst(true);
            return t;
        } else if (tok.view == "func")
            return l_parseFuncType(source);

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
