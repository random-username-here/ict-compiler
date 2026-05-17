#include "scl/ast/module.hpp"
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

    parseVarDeclLike(source, misc::TOK_COMMA, misc::TOK_RBRACE,
            [&func](misc::Token name, UPtr<Type> &&type, UPtr<Expr> &&initializer){
        if (initializer)
            throw misc::SourceError(name, "Default-valued arguments are not supported yet");
        func->args().createEnd(name, std::move(type));
    });
    auto cbrace = misc::tokenize(source, misc::TOKF_NONE);
    if (cbrace.type != ')')
        throw misc::SourceError(cbrace, "Expected argument list to be closed here");

    misc::Token tok;
    auto v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    if (tok.type != '{' && tok.type != ';') {
        func->returnType() = parseType(source);
        v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    } else {
        func->returnType() = PrimitiveType::create(PrimitiveType::VOID);
    }

    func->genDeclType();

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
    } else if (tok.view == "type") {
        auto name = misc::tokenize(source, misc::TOKF_NONE);
        if (name.type != misc::TOK_NAME)
            throw misc::SourceError(name, "New type's name expected");
        auto t = parseType(source);
        auto end = misc::tokenize(source, misc::TOKF_NONE);
        if (end.type != ';')
            throw misc::SourceError(end, "`;` expected after typedef");
        into->entries().createEnd<TypeDef>(name, std::move(t));
    } else if (tok.view == "var") {
        auto decl = GlobalVarBlock::create();
        parseVarDeclLike(source, misc::TOK_COMMA, misc::TOK_SEMICOL,
                [&decl](misc::Token name, UPtr<Type> &&type, UPtr<Expr> &&init){
            decl->vars().createEnd(name, std::move(type), std::move(init));
        });
        auto end = misc::tokenize(source, misc::TOKF_NONE);
        if (end.type != ';')
            throw misc::SourceError(end, "`;` expected after variable declaration");
        into->entries().push(std::move(decl));
    } else {
        throw misc::SourceError(tok, "Function expected");
    }
    return true;
}

};
