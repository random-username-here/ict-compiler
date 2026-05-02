#include "misclib/defs.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include "scl/ast/block.hpp"
#include "scl/parsing.hpp"

#define TAG "scl.parse"

namespace scl {

static void l_parseVarDecl(View &source, VarDeclStatement *into) {
    // Structure: name [type] [= expr] (, another / ;)
    auto name = misc::tokenize(source, misc::TOKF_NONE);
    if (name.type != misc::TOK_NAME)
        throw misc::SourceError(name, "Variable name expected");
    misc::verb(TAG) << "ParseVarDecl variable " << name.view;
    misc::Token tok;
    View v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    if (tok.type != ';' && tok.type != ',' && tok.view != "=") {
        throw misc::SourceError(name, "Variable types not supported yet"); // TODO
    } else {
        source = v;
    }
    misc::UPtr<Expr> initializer = nullptr;
    if (tok.view == "=") {
        initializer = parseExpr(source, { misc::TOK_COMMA, misc::TOK_SEMICOL });
        tok = misc::tokenize(source, misc::TOKF_NONE);
        if (tok.type != ',' && tok.type != ';')
            throw misc::SourceError(tok, "Expected `,` or `;` after variable initialization");
    }
    into->decls().createEnd(name.view, std::move(initializer));
    if (tok.type == ',') // another one
        l_parseVarDecl(source, into);
}

static UPtr<Statement> l_parseIf(View &source) {
    misc::Token brc;
    misc::tokenize(source, misc::TOKF_NONE, &brc);
    if (brc.type != '(')
        throw misc::SourceError(brc, "Expected `(` after `if`");
    auto expr = parseExprItem(source); // will be enclosed in `()`
    auto then = parseStatement(source);

    UPtr<Statement> otherwise = nullptr;
    misc::Token elseKw;
    View v = misc::tokenize(source, misc::TOKF_NONE, &elseKw);
    if (elseKw.type == misc::TOK_NAME && elseKw.view == "else") {
        source = v;
        otherwise = parseStatement(source);
    }
    return IfElse::create(std::move(expr), std::move(then), std::move(otherwise));
}

UPtr<Statement> parseStatement(View &source) {
    misc::Token tok;
    auto v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    misc::verb(TAG) << "ParseStatement token " << (char) tok.type << " `" << tok.view << "`";
    if (tok.type == ';') { 
        return nullptr;
    } else if (tok.type == misc::TOK_NAME && tok.view == "var") {
        source = v;
        auto ds = VarDeclStatement::create();
        l_parseVarDecl(source, ds.get());
        return ds;
    } else if (tok.type == misc::TOK_NAME && tok.view == "if") {
        source = v;
        return l_parseIf(source);
    } else if (tok.type == '{') {
        source = v;
        misc::Token maybeEnd;
        auto blk = Block::create();
        while (1) {
            View v = misc::tokenize(source, misc::TOKF_NONE, &maybeEnd);
            if (maybeEnd.type == '}') { source = v; break; }
            if (maybeEnd.type == misc::TOK_EOF)
                throw misc::SourceError(tok, "Block not closed");
            blk->items().push(parseStatement(source));
        }
        return blk;
    } else {
        auto expr = parseExpr(source, misc::TOK_SEMICOL);
        auto semicol = misc::tokenize(source, misc::TOKF_NONE);
        if (semicol.type != ';')
            throw misc::SourceError(semicol, "Semicolon after expression expected");
        return ExprInBlock::create(std::move(expr));
    }
}

};
