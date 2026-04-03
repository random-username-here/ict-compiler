#include "misclib/parse.hpp"
#include "misclib/dump_stream.hpp"
#include <alloca.h>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>

namespace misc {

bool numberIsIntegral(Number n) {
    return floorl(n) == n;
}

void SourceError::writeFormatted(std::ostream &out, std::string_view filename,
        View contents) const {

    // TODO: a prettier algorithm
    assert(contents.data() <= m_where.data());
    assert(m_where.data() <= contents.data() + contents.size());

    size_t line = 1, col = 1;
    size_t lineStartIdx = 0;
    for (const char *pos = contents.data(); pos < m_where.data(); ++pos) {
        if (*pos == '\n')
            line++, col = 1, lineStartIdx = pos - contents.begin() + 1;
        else
            col++;
    }

    size_t lineEndIdx = lineStartIdx;
    while (lineEndIdx < contents.size() && contents[lineEndIdx] != '\n')
        ++lineEndIdx;
    
    size_t hlBegin = lineStartIdx + col - 1, hlEnd = hlBegin + m_where.size();

    out << RED << BOLD << "error: " << RST << RED << m_what << RST << "\n";
    out << DGRAY << "─────┬─[" << RED << filename << YELLOW << ":" << line << ":" << col << DGRAY << "]" << "\n";
    out << std::setw(4) << line << " │ " 
        << RST << contents.substr(lineStartIdx, std::min(hlBegin, lineEndIdx) - lineStartIdx) 
        << RED << contents.substr(hlBegin, hlBegin == lineEndIdx ? 0 : hlEnd - hlBegin)
        << RST << contents.substr(hlEnd, hlEnd > lineEndIdx ? 0 : lineEndIdx - hlEnd) // in case \n is highlighted
        << "\n";
    out << DGRAY << "    " << " ┘ " << RED;
    for (int i = 0; i < col-1; ++i)
        out << ' ';
    for (int i = 0; i < m_where.size(); ++i)
        out << "^";
    out << RST << "\n";
}

bool startsWith(View view, View start) {
    if (view.size() < start.size())
        return false;
    return view.substr(0, start.size()) == start;
}

bool endsWith(View view, View end) {
    if (view.size() < end.size())
        return false;
    return view.substr(view.size() - end.size(), view.size()) == end;
}

/// Skip comments and spaces
View withoutWhitespaces(View view, int flags) {
may_skip_more:
    if (view.empty())
        return view;
    if (isspace(view[0]) && (view[0] != '\n' || !(flags & TOKF_NL))) {
        view.remove_prefix(1);
        goto may_skip_more;
    }
    if (startsWith(view, "//")) {
        while (!view.empty() && view[0] != '\n')
            view.remove_prefix(1);
        goto may_skip_more;
    }
    if (startsWith(view, "/*")) {
        view.remove_prefix(2);
        while (!view.empty() && !startsWith(view, "*/"))
            view.remove_prefix(1);
        view.remove_prefix(2);
        goto may_skip_more;
    }
    return view;
}

static View takeToken(View view, size_t len, TokenType type, Token *tok) {
    if (tok)
        *tok = Token { view.substr(0, len), type };
    return view.substr(len);
}

static View takeWhile(View view, bool (*predicate)(char ch), TokenType type, Token *tok) {
    assert(!view.empty());
    size_t cnt = 0;
    while (cnt < view.size() && predicate(view[cnt]))
        ++cnt;
    return takeToken(view, cnt, type, tok);
}

static View takeString(View view, Token *tok) {
    assert(!view.empty());
    assert(view[0] == '"');
    
    size_t cnt = 1;
    bool esc = false;
    while (cnt < view.size()) {
        if (esc) esc = false;
        else if (view[cnt] == '\\') esc = true;
        else if (view[cnt] == '"') break;
        ++cnt;
    }
    return takeToken(view, cnt+1, TOK_STR, tok);
}

static bool isNumber(char ch) {
    // commented out cases are in other ranges
    switch (ch) {
        case '0'...'9':
        case '.':
        case 'x': /*case 'b':*/ // hex & binary
        /*case 'e':*/ // float exp
        case 'a'...'f': //hex
        case 'A'...'F':
            return true;
        default:
            return false;
    }
}

static bool isOperator(char ch) {
    switch (ch)  {
        case '+': case '-':
        case '*': case '/':
        case '=':
        case '<': case '>':
        case '&': case '|':
        case '~': case '!':
        case '^':
            return true;
        default:
            return false;
    }
}

static bool isName(char ch) {
    switch (ch) {
        case 'a'...'z':
        case 'A'...'Z':
        case '0'...'9':
        case '_': case '$': case '#':
            return true;
        default:
            return false;
    }
}

static bool isDotName(char ch) {
    if (ch == '.' || ch == '@' || ch == '%')
        return true;
    return isName(ch);
}

// Grab one token from the stream
View tokenize(View view, int flags, Token *tok) {
    view = withoutWhitespaces(view, flags);
    if (view.empty())
        return takeToken(view, 0, TOK_EOF, tok);
    switch (view[0]) {
        case '(': case ')':
        case '[': case ']':
        case '{': case '}':
        case ';': case ':':
        case ',':
        case '\n':
            return takeToken(view, 1, (TokenType) view[0], tok);
        case '"':
            return takeString(view, tok);
        case '0'...'9':
            return takeWhile(view, isNumber, TOK_NUM, tok);
        case '.': case '@':
            if (!(flags & TOKF_DOTNAME))
                return takeToken(view, 1, (TokenType) view[0], tok);
            // else fallthrough to name
        default:
            bool (*nameChk)(char) = isName;
            if (flags & TOKF_DOTNAME) 
                nameChk = isDotName;

            if (nameChk(view[0]))
                return takeWhile(view, nameChk, TOK_NAME, tok);
            else if (isOperator(view[0]))
                return takeWhile(view, isOperator, TOK_OP, tok);
            else
                throw SourceError(view.substr(0, 1), "Character unknown to the tokenizer");
    }
}

#define MAX_FILENAME_LEN 4096

std::string readWholeFile(std::string_view name) {

    if (name.size() > MAX_FILENAME_LEN)
        throw std::ios_base::failure("Too long filename");
    char *buf = (char*) alloca(name.size() + 1);
    memcpy(buf, name.data(), name.size());
    buf[name.size()] = '\0';
    
    std::ifstream ifs;
    ifs.exceptions(ifs.exceptions() | std::ios::failbit);
    ifs.open(buf);
    
    ifs.seekg(0, std::ios_base::end);
    std::string data;
    data.resize(ifs.tellg());

    ifs.seekg(0, std::ios_base::beg);
    ifs.read(data.data(), data.size());

    return data;
}

static int hexdec(char ch) {
    switch (ch) {
        case '0' ... '9':
            return ch - '0';
        case 'a' ... 'f':
            return ch - 'a' + 10;
        case 'A' ... 'F':
            return ch - 'A' + 10;
        default:
            return 0;
    }
}

std::string Token::decodeStr() const {
    std::string res;
    for (size_t i = 0; i < view.size(); ++i) {
        if (view[i] == '\\') {
            if (i == view.size() - 1) break;
            char ch = view[i+1];
            switch(view[i+1]) {
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case 'r': ch = '\r'; break;
                case '0': ch = '\0'; break;
                case 'x': // \xNN
                    if (i+3 < view.size() && isxdigit(view[i+2])
                            && isxdigit(view[i+3])) {
                        ch = hexdec(view[i+2]) * 16 + hexdec(view[i+3]);
                        i += 2;
                    }
                    break;
            }
            res += ch;
            i += 2;
        } else {
            res += view[i];
        }
    }
    return res;
}

Number Token::decodeNum() const {
    // we have a string_view, so no strto-class functions
    Number result = 0;
    Number mul = 0.1, exp = 0;
   
    if (startsWith(view, "0b") || startsWith(view, "0x")) {
        int base = view[1] == 'b' ? 2 : 16;
        for (size_t i = 2; i < view.size(); ++i) {
            char ch = view[i];
            if (base == 2 && (ch == '0' || ch == '1'))
                result = result * 2 + (ch - '0');
            else if (base == 16 && isxdigit(ch))
                result = result * 16 + hexdec(ch);
            else
                throw SourceError(view.substr(i, 1), "Unexpected character in hex/bin number");
        }
        return result;
    }

    size_t i = 0;

    if (view[i] == '.') goto dot;

whole: // whole part

    while (i < view.size() && view[i] != '.' && view[i] != 'e') {
        if (!isdigit(view[i]))
            throw SourceError(view.substr(i, 1), "Unexpected character in number");
        result = result * 10 + view[i] - '0';
        ++i;
    }

    if (i == view.size()) goto end;
    if (view[i] == 'e') goto exp;

dot: // after dot

    ++i; // skip dot

    while (i < view.size() && view[i] != 'e') {
        if (view[i] == '.')
            throw SourceError(view.substr(i, 1), "Duplicate dot in number");
        else if (!isdigit(view[i]))
            throw SourceError(view.substr(i, 1), "Unexpected character in number");
        result += (view[i] - '0') * mul;
        mul /= 10;
        ++i;
    }

    if (i == view.size()) goto end;

exp: // exponent

    ++i;

    while (i < view.size()) {
        if (view[i] == '.')
            throw SourceError(view.substr(i, 1), "Non-integer exponents not supported");
        else if (!isdigit(view[i]))
            throw SourceError(view.substr(i, 1), "Unexpected character in number");
        exp = exp * 10 + view[i] - '0';
        ++i;
    }

    result *= std::pow(10, exp);

end:

    return result;

}

};
