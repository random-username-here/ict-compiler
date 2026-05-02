#include "scl/parsing.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include "scl/ast/expr.hpp"
#include <initializer_list>

#define TAG "scl.parse"

namespace scl {

static UPtr<Expr> l_maybeMakePack(UPtr<Expr> &&expr) {
    // unpack a long chain, like (a, (b, (c, ...)))
    auto op = dynamic_cast<Binary*>(expr.get());
    if (!op || op->kind() != BIN_COMMA) return std::move(expr);
    auto pack = ArgPack::create();
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
    misc::verb(TAG) << "ParseExprItem first token " << (char) tok.type << " `" << tok.view << "`\n";
    if (tok.type == misc::TOK_NUM) {
        return scl::Number::create(tok.decodeNum());
    } else if (tok.type == misc::TOK_NAME) {
        return scl::Name::create(tok);
    } else if (tok.type == '(') {
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
    OT_UNR = 1, // unary, rtl
    OT_BIN = 2, // bin, rtl
    OT_RAS = 4, // binary, rtl => 
    OT_LAS = 8
};

struct OpDef { int flags; View text; int type; int prior; };

static const OpDef l_spaceOp = { OT_BIN | OT_RAS, "(space)", BIN_SPACE, 0 };

static const OpDef l_ops[] = {
    // reflect c++'ses style
    { OT_UNR | OT_RAS, "-", UN_NEG, 2 },
    { OT_UNR | OT_RAS, "*", UN_DEREF, 2 },

    { OT_UNR | OT_RAS, "&", UN_REF, 3 },
    
    { OT_BIN | OT_LAS, "*", BIN_MUL, 5 },
    { OT_BIN | OT_LAS, "/", BIN_DIV, 5 },
    { OT_BIN | OT_LAS, "%", BIN_MOD, 5 },

    { OT_BIN | OT_LAS, "+", BIN_ADD, 6 },
    { OT_BIN | OT_LAS, "-", BIN_SUB, 6 },
    
    { OT_BIN | OT_LAS, "<", BIN_LT, 9 },
    { OT_BIN | OT_LAS, ">", BIN_GT, 9 },
    { OT_BIN | OT_LAS, "<=", BIN_LE, 9 },
    { OT_BIN | OT_LAS, ">=", BIN_GE, 9 },

    { OT_BIN | OT_LAS, "==", BIN_EQ, 10 },
    { OT_BIN | OT_LAS, "!=", BIN_NEQ, 10 },

    { OT_BIN | OT_RAS, ",", BIN_COMMA, 17 }, // right-associative, unlike C++
};

static const OpDef *l_findOp(View s, bool unary) {
    for (size_t i = 0; i < sizeof(l_ops)/sizeof(OpDef); ++i) {
        if (bool(l_ops[i].flags & OT_UNR) != unary) continue;
        if (l_ops[i].text != s) continue;
        return &l_ops[i];
    }
    return nullptr;
}

void l_unstackOp(std::vector<UPtr<Expr>> &valStack, std::vector<const OpDef*> &opStack) {
    auto top = opStack.back();
    misc::verb(TAG) << "Unstack " << top->text << "\n";
    if (top->flags & OT_UNR) {
        auto v = std::move(valStack.back());
        valStack.pop_back();
        valStack.push_back(Unary::create((UnaryKind) top->type, std::move(v)));
    } else {
        auto b = std::move(valStack.back()); valStack.pop_back();
        auto a = std::move(valStack.back()); valStack.pop_back();
        valStack.push_back(Binary::create((BinaryKind) top->type, std::move(a), std::move(b)));
    }
    opStack.pop_back();
}

static bool l_initListHas(std::initializer_list<misc::TokenType> il, misc::TokenType t) {
    for (auto i : il)
        if (i == t) return true;
    return false;
}

UPtr<Expr> parseExpr(View &source, std::initializer_list<misc::TokenType> terminal) {
    std::vector<UPtr<Expr>> valStack;
    std::vector<const OpDef*> opStack;

    misc::Token tok;
    View pos = misc::tokenize(source, misc::TOKF_NONE, &tok);
    bool unaryExpected = true;
    while (!l_initListHas(terminal, tok.type)) {
        misc::verb(TAG) << "ParseExpr token " << (char) tok.type << " `" << tok.view << "`, ue = " << unaryExpected << "\n";
        if (tok.type == misc::TOK_OP || tok.type == ',') {
            source = pos;
            auto def = l_findOp(tok.view, unaryExpected);
            if (!def)
                throw misc::SourceError(tok, "Unknown operator");
            while (!opStack.empty() && (
                        opStack.back()->prior < def->prior 
                        || (opStack.back()->prior < def->prior && (def->flags & OT_LAS)))) {
                l_unstackOp(valStack, opStack);
            }
            opStack.push_back(def);
            unaryExpected = true;
        } else {
            if (!unaryExpected)
                opStack.push_back(&l_spaceOp);
            valStack.push_back(parseExprItem(source));
            unaryExpected = false;
        }
        pos = misc::tokenize(source, misc::TOKF_NONE, &tok);
    }
    if (unaryExpected)
        throw misc::SourceError(tok, "Value expected afterwards");
    while (!opStack.empty())
        l_unstackOp(valStack, opStack);
    return valStack.empty() ? nullptr : std::move(valStack[0]);
}


};
