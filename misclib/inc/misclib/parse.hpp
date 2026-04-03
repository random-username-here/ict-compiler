/**
 * \file
 * \brief A simple tokenizer
 */
#pragma once
#include <exception>
#include <string>
#include <string_view>
#include "defs.hpp"

namespace misc {

/**
 * Value of TOK_NUM
 */
using Number = long double;

bool numberIsIntegral(Number n);

/**
 * \brief Type of the token.
 * Can be cast to `char`. All single-character
 * tokens have their type equal to their character.
 */
enum TokenType {

    // special tokens
    TOK_EOF         = 0,

    // token classes
    TOK_NUM         = '0',
    TOK_NAME        = 'n',
    TOK_OP          = '?',
    TOK_STR         = '"',

    // single-character tokens
    TOK_LBRACE      = '(',
    TOK_RBRACE      = ')',
    TOK_LBOX        = '[',
    TOK_RBOX        = ']',
    TOK_LFIG        = '{',
    TOK_RFIG        = '}',
    TOK_SEMICOL     = ';',
    TOK_DOTS        = ':',
    TOK_COMMA       = ',',
    TOK_DOT         = '.',
    TOK_NEWLINE     = '\n',

};

/**
 * A token.
 */
struct Token {
    View view;
    TokenType type;

    std::string decodeStr() const;
    Number decodeNum() const;
};

/**
 * Flags used to control parsing functions.
 */
enum TokenFlags {
    TOKF_NONE    = 0,
    TOKF_NL      = 1, // newline as separate token
    TOKF_DOTNAME = 2, // `.`, `@` are valid identifier characters
};

/// Check if view starts with given other view
bool startsWith(View view, View start);

/// Check if view ends with another view
bool endsWith(View view, View end);

/// Skip comments and spaces
View withoutWhitespaces(View view, int flags);

inline void skipWS(View &view, int flags) {
    view = withoutWhitespaces(view, flags);
}

// Grab one token from the stream
View tokenize(View view, int flags, Token *token);

inline Token tokenize(View &view, int flags) {
    Token t;
    view = tokenize(view, flags, &t);
    return t;
}

// Read that whole file into string
std::string readWholeFile(std::string_view name);

class SourceError : public std::exception {
	View m_where;
	std::string m_what;
public:
	SourceError(View where, View what) :m_where(where), m_what(what) {}
	SourceError(Token where, View what) :m_where(where.view), m_what(what) {}
	
	View where() const { return m_where; }
	const char *what() const noexcept { return m_what.c_str(); }
	
	void writeFormatted(
			std::ostream &out, std::string_view filename,
        	View contents
	) const;
};

/**
 * \brief A base class for AST nodes
 * This contains token from which the node was created.
 */
class WithToken {
    misc::Token m_token;
public:
    WithToken() {}
    WithToken(misc::Token view) :m_token(view) {}
    const misc::Token &token() const { return m_token; }
};

};
