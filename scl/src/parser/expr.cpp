#include "scl/parsing.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include "scl/ast/expr.hpp"
#include <cstdint>
#include <initializer_list>

#define TAG "scl.parse"

namespace scl {

static UPtr<Expr> l_maybeMakePack(UPtr<Expr> &&expr) {
    // unpack a long chain, like (a, (b, (c, ...)))
    auto op = dynamic_cast<Binary*>(expr.get());
    if (!op || op->kind() != BIN_COMMA) return std::move(expr);
    auto pack = ArgPack::create(op->token()); // TODO: capture all pack inside
    while (1) {
        auto op = dynamic_cast<Binary*>(expr.get());
        if (!op || op->kind() != BIN_COMMA) break;
        pack->items().push(op->left()->replace(nullptr));
        expr = op->right()->replace(nullptr);
    }
    pack->items().push(std::move(expr));
    return pack;
}

UPtr<Expr> parseExprItem(View &source) {
    auto tok = misc::tokenize(source, misc::TOKF_NONE);
    //misc::verb(TAG) << "ParseExprItem first token " << (char) tok.type << " `" << tok.view << "`\n";
    if (tok.type == misc::TOK_CHAR) {
        auto sv = tok.decodeStr();
        misc::verb(TAG) << "str `" << sv << "` -- " << sv.size() << '\n';
        if (sv.size() > 8)
            throw misc::SourceError(tok, "Characters are at maximum 8 bytes long");
        uint64_t v = 0;
        for (size_t i = 0; i < sv.size(); ++i)
            v += ((uint64_t) sv[i]) << (8 * i);
        return scl::Number::create(v);
    } else if (tok.type == misc::TOK_STR) {
        return scl::String::create(tok);
    } else if (tok.type == misc::TOK_NUM) {
        return scl::Number::create(tok);
    } else if (tok.type == misc::TOK_NAME) {
        return scl::Name::create(tok);
    } else if (tok.type == '(') {
        auto t = misc::tokenize(source, misc::TOKF_NONE, &tok);
        if (tok.type == ')') {
            source = t;
            return ArgPack::create(tok);
        }
        auto v = l_maybeMakePack(parseExpr(source, misc::TOK_RBRACE));
        auto ebrace = misc::tokenize(source, misc::TOKF_NONE);
        if (ebrace.type == misc::TOK_EOF)
            throw misc::SourceError(tok, "Brace not closed");
        else if (ebrace.type != ')')
            throw misc::SourceError(ebrace, "Expected an `)` here");
        return v;
    } else {
        throw misc::SourceError(tok, "Unknown token");
    }
}

enum OpType {
    OT_UNR = 1, // prefix unary
    OT_BIN = 2, // normal binary
    OT_RAS = 4, // right-associative
    OT_LAS = 8, // left-associative
    OT_SUF = 16 // suffix unary
};

struct OpDef { int flags; View text; int type; int prior; };

static const OpDef l_spaceOp = { OT_BIN | OT_RAS, "(space)", BIN_SPACE, 0 };

static const OpDef l_ops[] = {
    // reflect c++'ses style
    // based on https://en.cppreference.com/cpp/language/operator_precedence

    { OT_SUF | OT_LAS, "++", UN_INC_POST, 1 },
    { OT_SUF | OT_LAS, "--", UN_DEC_POST, 1 },

    { OT_UNR | OT_RAS, "++", UN_INC_PRE, 2 },
    { OT_UNR | OT_RAS, "--", UN_DEC_PRE, 2 },
    { OT_UNR | OT_RAS, "!", UN_NOT, 2 },
    { OT_UNR | OT_RAS, "~", UN_INV, 2 },
    { OT_UNR | OT_RAS, "-", UN_NEG, 2 },
    { OT_UNR | OT_RAS, "*", UN_DEREF, 2 },

    { OT_UNR | OT_RAS, "&", UN_REF, 3 },
    
    { OT_BIN | OT_LAS, "*", BIN_MUL, 5 },
    { OT_BIN | OT_LAS, "/", BIN_DIV, 5 },
    { OT_BIN | OT_LAS, "%", BIN_MOD, 5 },

    { OT_BIN | OT_LAS, "+", BIN_ADD, 6 },
    { OT_BIN | OT_LAS, "-", BIN_SUB, 6 },
    
    { OT_BIN | OT_LAS, ">>", BIN_RSH, 7 },
    { OT_BIN | OT_LAS, "<<", BIN_LSH, 7 },
    
    { OT_BIN | OT_LAS, "<", BIN_LT, 9 },
    { OT_BIN | OT_LAS, ">", BIN_GT, 9 },
    { OT_BIN | OT_LAS, "<=", BIN_LE, 9 },
    { OT_BIN | OT_LAS, ">=", BIN_GE, 9 },

    { OT_BIN | OT_LAS, "==", BIN_EQ, 10 },
    { OT_BIN | OT_LAS, "!=", BIN_NEQ, 10 },
    
    { OT_BIN | OT_LAS, "&", BIN_BAND, 11 },
    { OT_BIN | OT_LAS, "^", BIN_BXOR, 12 },
    { OT_BIN | OT_LAS, "|", BIN_BOR, 13 },
    
    { OT_BIN | OT_LAS, "&&", BIN_AND, 14 },

    { OT_BIN | OT_LAS, "||", BIN_OR, 15 },
    
    { OT_BIN | OT_RAS, "=", BIN_ASSIGN, 16 },
    { OT_BIN | OT_RAS, "+=", BIN_IADD, 16 },
    { OT_BIN | OT_RAS, "-=", BIN_ISUB, 16 },
    { OT_BIN | OT_RAS, "*=", BIN_IMUL, 16 },
    { OT_BIN | OT_RAS, "/=", BIN_IDIV, 16 },
    { OT_BIN | OT_RAS, "%=", BIN_IMOD, 16 },
    { OT_BIN | OT_RAS, ">>=", BIN_IRSH, 16 },
    { OT_BIN | OT_RAS, "<<=", BIN_ILSH, 16 },
    { OT_BIN | OT_RAS, "^=", BIN_IBXOR, 16 },
    { OT_BIN | OT_RAS, "&=", BIN_IBAND, 16 },
    { OT_BIN | OT_RAS, "|=", BIN_IBOR, 16 },

    { OT_BIN | OT_RAS, ",", BIN_COMMA, 17 }, // right-associative, unlike C++
};

static const OpDef *l_findOp(View s, bool unary) {
    for (size_t i = 0; i < sizeof(l_ops)/sizeof(OpDef); ++i) {
        // suffix unary's are counted as not unary here
        if (bool(l_ops[i].flags & OT_UNR) != unary) continue;
        if (l_ops[i].text != s) continue;
        return &l_ops[i];
    }
    return nullptr;
}

void l_unstackOp(std::vector<UPtr<Expr>> &valStack, std::vector<const OpDef*> &opStack, std::vector<misc::Token> &opTokStack) {
    auto top = opStack.back();
    //misc::verb(TAG) << "Unstack " << top->text << "\n";
    if ((top->flags & OT_UNR) || (top->flags & OT_SUF)) {
        auto v = std::move(valStack.back());
        valStack.pop_back();
        valStack.push_back(Unary::create(opTokStack.back(), (UnaryKind) top->type, std::move(v)));
    } else {
        auto b = std::move(valStack.back()); valStack.pop_back();
        auto a = std::move(valStack.back()); valStack.pop_back();
        valStack.push_back(Binary::create(opTokStack.back(), (BinaryKind) top->type, std::move(a), std::move(b)));
    }
    opTokStack.pop_back();
    opStack.pop_back();
}

static bool l_initListHas(std::initializer_list<misc::TokenType> il, misc::TokenType t) {
    for (auto i : il)
        if (i == t) return true;
    return false;
}

static bool l_shouldUnstack(const std::vector<const OpDef*> &opStack, const OpDef *def) {
    if (opStack.empty()) return false;
    auto top = opStack.back();
    if ((def->flags & OT_SUF) && def->prior > top->prior)
        return false; // first apply suffix, after that do that operator
    if ((def->flags & OT_UNR) && (top->flags & OT_UNR))
        return false; // both waiting for value
    if (top->prior < def->prior) return true;
    if (top->prior == def->prior && (def->flags & OT_LAS)) return true;
    return false;
}

UPtr<Expr> parseExpr(View &source, std::initializer_list<misc::TokenType> terminal) {
    std::vector<UPtr<Expr>> valStack;
    std::vector<const OpDef*> opStack;
    std::vector<misc::Token> opTokStack;

    misc::Token tok;
    View pos = misc::tokenize(source, misc::TOKF_NONE, &tok);
    bool unaryExpected = true;
    while (!l_initListHas(terminal, tok.type)) {
        //misc::verb(TAG) << "ParseExpr token " << (char) tok.type << " `" << tok.view << "`, ue = " << unaryExpected << "\n";
        if (tok.type == misc::TOK_OP || tok.type == ',') {
            source = pos;
            auto def = l_findOp(tok.view, unaryExpected);
            if (!def)
                throw misc::SourceError(tok, "Unknown operator");
            while (l_shouldUnstack(opStack, def))
                l_unstackOp(valStack, opStack, opTokStack);
            opTokStack.push_back(tok);
            opStack.push_back(def);
            if (def->flags & OT_SUF) {
                l_unstackOp(valStack, opStack, opTokStack);
                unaryExpected = false;
            } else {
                unaryExpected = true;
            }
        } else {
            if (!unaryExpected) {
                opStack.push_back(&l_spaceOp);
                opTokStack.push_back(tok); // obrace or argument
            }
            valStack.push_back(parseExprItem(source));
            unaryExpected = false;
        }
        pos = misc::tokenize(source, misc::TOKF_NONE, &tok);
    }
    if (unaryExpected)
        throw misc::SourceError(tok, "Value expected afterwards");
    while (!opStack.empty())
        l_unstackOp(valStack, opStack, opTokStack);
    //misc::verb(TAG) << "ParseExpr end, vs has " << valStack.size() << " items\n";
    return valStack.empty() ? nullptr : std::move(valStack[0]);
}


};
