#include "misclib/defs.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include "scl/ast/block.hpp"
#include "scl/parsing.hpp"

#define TAG "scl.parse"

namespace scl {

void parseVarDeclLike(
        View &source, misc::TokenType separator, misc::TokenType end,
        std::function<void(misc::Token name, UPtr<Type> &&type, UPtr<Expr> &&initializer)> callback
) {
    misc::Token name;
    View v = misc::tokenize(source, misc::TOKF_NONE, &name);
    if (name.type == end || name.type == misc::TOK_EOF) return;
    source = v;
    if (name.type != misc::TOK_NAME)
        throw misc::SourceError(name, "Name expected");
    misc::Token tok;
    UPtr<Type> type = nullptr;
    v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    if (tok.type != end && tok.type != separator && tok.view != "=") {
        type = parseType(source);
        v = misc::tokenize(source, misc::TOKF_NONE, &tok);
        if (tok.type != separator && tok.type != end && tok.view != "=")
            throw misc::SourceError(tok, "Expected assignment or end after type");
        if (tok.type != end)
            source = v;
    } else if (tok.type != end) {
        source = v; // eat token
    }
    misc::UPtr<Expr> initializer = nullptr;
    if (tok.view == "=") {
        initializer = parseExpr(source, { separator, end });
        v = misc::tokenize(source, misc::TOKF_NONE, &tok);
        if (tok.type != separator && tok.type != end)
            throw misc::SourceError(tok, "Expected next item after initializer");
        if (tok.type != end)
            source = v;
    }
    callback(name, std::move(type), std::move(initializer));
    if (tok.type == separator) // another one
        parseVarDeclLike(source, separator, end, callback);
}

void parseVarDecls(View &source, VarDeclStatement *into) {
    // this allows for stupid `var;` statement what does nothing
    parseVarDeclLike(source, misc::TOK_COMMA, misc::TOK_SEMICOL,
            [into](misc::Token name, UPtr<Type> &&type, UPtr<Expr> &&initializer){
        into->decls().createEnd(name, std::move(type), std::move(initializer));
    });
    auto end = misc::tokenize(source, misc::TOKF_NONE);
    if (end.type != ';')
        throw misc::SourceError(end, "`;` expected after variable declarations");
}

static UPtr<Statement> l_parseIf(View &source, misc::Token tok) {
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
    return IfElse::create(tok, std::move(expr), std::move(then), std::move(otherwise));
}

static UPtr<Statement> l_parseWhile(View &source, misc::Token tok) {
    misc::Token brc;
    misc::tokenize(source, misc::TOKF_NONE, &brc);
    if (brc.type != '(')
        throw misc::SourceError(brc, "Expected `(` after `while`");
    auto expr = parseExprItem(source); // will be enclosed in `()`
    auto body = parseStatement(source);
    return While::create(tok, std::move(expr), std::move(body));
}

UPtr<Statement> parseStatement(View &source) {
    misc::Token tok;
    auto v = misc::tokenize(source, misc::TOKF_NONE, &tok);
    misc::verb(TAG) << "ParseStatement token " << (char) tok.type << " `" << tok.view << "`";
    if (tok.type == ';') { 
        source = v;
        return nullptr;
    } else if (tok.type == misc::TOK_NAME && tok.view == "var") {
        source = v;
        auto ds = VarDeclStatement::create(tok);
        parseVarDecls(source, ds.get());
        return ds;
    } else if (tok.type == misc::TOK_NAME && tok.view == "return") {
        source = v;
        v = misc::tokenize(source, misc::TOKF_NONE, &tok);
        misc::UPtr<Expr> expr;
        if (tok.type != misc::TOK_SEMICOL)
            expr = parseExpr(source, misc::TOK_SEMICOL);
        auto semicol = misc::tokenize(source, misc::TOKF_NONE);
        if (semicol.type != misc::TOK_SEMICOL)
            throw misc::SourceError(semicol, "Semicolon expected");
        return ReturnStatement::create(tok, std::move(expr));
    } else if (tok.type == misc::TOK_NAME && tok.view == "while") {
        source = v;
        return l_parseWhile(source, tok);
    } else if (tok.type == misc::TOK_NAME && tok.view == "if") {
        source = v;
        return l_parseIf(source, tok);
    } else if (tok.type == '{') {
        source = v;
        misc::Token maybeEnd;
        auto blk = Block::create(tok);
        while (1) {
            View v = misc::tokenize(source, misc::TOKF_NONE, &maybeEnd);
            if (maybeEnd.type == '}') { source = v; break; }
            if (maybeEnd.type == misc::TOK_EOF)
                throw misc::SourceError(tok, "Block not closed");
            auto item = parseStatement(source);
            if (!item) continue;
            else blk->items().push(std::move(item));
        }
        return blk;
    } else {
        auto expr = parseExpr(source, misc::TOK_SEMICOL);
        if (!expr) return nullptr;
        auto semicol = misc::tokenize(source, misc::TOKF_NONE);
        if (semicol.type != ';')
            throw misc::SourceError(semicol, "Semicolon after expression expected");
        auto tok = expr->token();
        return ExprInBlock::create(tok, std::move(expr)); // TODO: better token to refer by
    }
}

};
