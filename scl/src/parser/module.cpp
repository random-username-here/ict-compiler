#include "misclib/parse.hpp"
#include "scl/ast/type.hpp"
#include "scl/parsing.hpp"

namespace scl {

static void l_parseFunc(View &source, Module *into) {
    auto name = misc::tokenize(source, misc::TOKF_NONE);
    if (name.type != misc::TOK_NAME)
        throw misc::SourceError(name, "Function name expected");
    auto obrace = misc::tokenize(source, misc::TOKF_NONE);
    if (obrace.type != '(')
        throw misc::SourceError(obrace, "Function argument list expected (obrace)");
    auto func = Function::create(name);
    while (1) {
        auto name = misc::tokenize(source, misc::TOKF_NONE);
        if (name.type == ')') break;
        if (name.type != misc::TOK_NAME)
            throw misc::SourceError(name, "Argument name expected");
        misc::Token tok;
        auto v = misc::tokenize(source, misc::TOKF_NONE, &tok);
        UPtr<Type> type = nullptr;
        if (tok.type != ',' && tok.type != ')') {
            type = parseType(source);
            v = misc::tokenize(source, misc::TOKF_NONE, &tok);
        } else {
            if (func->args().size() == 0)
                throw misc::SourceError(name, "Argument type can be ommited only when there is arg before to take type from");
            type = func->args().last()->type()->copy();
        }
        func->args().createEnd(name, std::move(type));
        source = v;
        if (tok.type == ')')
            break;
        else if (tok.type == ',')
            continue;
        else
            throw misc::SourceError(tok, "Expected `)` or `,` in argument list");
    }
    func->returnType() = parseType(source);
    func->genDeclType();
    misc::Token tok;
    auto v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    if (tok.type != '{' && tok.type != ';') {
        func->type() = parseType(source);
        v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    }
    if (tok.type == ';')
        source = v;
    else
        func->body() = parseStatement(source);
    into->entries().push(std::move(func));
}

bool parseTopLevel(View &source, Module *into) {
    auto tok = misc::tokenize(source, misc::TOKF_NONE);
    if (tok.type == misc::TOK_EOF)
        return false;
    if (tok.type != misc::TOK_NAME)
        throw misc::SourceError(tok, "Expected a top-level keyword");
    if (tok.view == "func") {
        l_parseFunc(source, into);
    } else {
        throw misc::SourceError(tok, "Function expected");
    }
    return true;
}

};
