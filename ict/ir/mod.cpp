/**
 * \file
 * \brief ICT frontend for IR files
 * \todo Allow backward references
 */
#include "ict/mod.hpp"
#include "ict/ir.hpp"
#include "misclib/dump_stream.hpp"
#include "misclib/parse.hpp"
#include <cctype>
#include <unordered_map>

#define TAG "ict.ir"

using misc::View;
using misc::UPtr;

class IrFrontend : 
        public ict::Frontend
{
public:
    View id() const override { return "ict.ir.frontend"; }
    std::string_view brief() const override { return "ICT frontend for reading IR from file"; }
    View langName() const override { return "ict-ir"; }

    bool takesFile(View name) const override {
        return misc::endsWith(name, ".ict");
    }

    using GlobalNameTable = std::unordered_map<View, ict::Function*>;
    using BlockNameTable = std::unordered_map<View, ict::Block*>;
    using VRegNameTable = std::unordered_map<View, ict::Op*>;

    UPtr<ict::Op> parseOp(
            ict::Manager *mgr,
            misc::Token name, View &source
    ) const {
        misc::verb(TAG) << "    Found operator " << misc::ACCENT << name.view << misc::RST << "\n";

        ict::OpKind kind = ict::OpKind::parse(name.view);
        if (kind == ict::UNKNOWN_OP)
            kind = mgr->backend()->str2lowOp(name.view);
        if (kind == ict::UNKNOWN_OP)
            throw misc::SourceError(name, "Operator name is not known in common IR or in low-level IR");

        return ict::Op::create(kind);
    }

    View getArgsView(
            ict::Manager *mgr,
            misc::Token blockBrace, View &source
    ) const {
        auto argsBegin = source.data();
        while (1) {
            auto argTok = misc::tokenize(source, misc::TOKF_DOTNAME);
            if (argTok.type == misc::TOK_EOF)
                throw misc::SourceError(blockBrace, "Block not closed");
            if (argTok.type == ';')
                break;
        }
        return View(argsBegin, source.data() - argsBegin);
    }

    void parseOpArgs(
            ict::Manager *mgr,
            ict::Op *op,
            View source,
            const GlobalNameTable &gnt,
            const BlockNameTable &bnt,
            const VRegNameTable &vrnt
    ) const {
        while (1) {
            misc::Token tok = misc::tokenize(source, misc::TOKF_DOTNAME);
            if (tok.type == misc::TOK_EOF || tok.type == ';')
                break;

            if (tok.type == misc::TOK_NUM) {
                auto val = tok.decodeNum();
                if (!misc::numberIsIntegral(val))
                    throw misc::SourceError(tok, "IR works only with integers for now");
                op->push(ict::ConstArg::create(val));
            } else if (tok.type == misc::TOK_NAME) {
                if (tok.view[0] == '@') {
                    // TODO: write this part when calls are added
                    throw misc::SourceError(tok, "Calls are not supported yet by IR");
                } else if (tok.view[0] == '%') {
                    auto name = tok.view.substr(1);
                    if (bnt.count(name)) {
                        op->push(ict::BlockArg::create(bnt.at(name)));
                    } else if (vrnt.count(name)) {
                        op->push(ict::VRegArg::create(vrnt.at(name)));
                    } else {
                        throw misc::SourceError(tok, "Unknown local name");
                    }
                } else {
                    throw misc::SourceError(tok, "Expected %-name or @-name here");
                }
            } else {
                throw misc::SourceError(tok, "Expected a valid operator argument");
            }
        }
    }

    UPtr<ict::Function> parseFunction(ict::Manager *mgr, View &source, const GlobalNameTable &gnt) const {
        auto name = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (name.type != misc::TOK_NAME)
            throw misc::SourceError(name, "Function implementation must have function name to implement");
        if (name.view[0] != '@')
            throw misc::SourceError(name, "Function name must start with `@`");
        misc::verb(TAG) << "Found function " << misc::ACCENT << name.view << misc::RST << "\n";
        auto func = ict::Function::create(name.view.substr(1));
        auto block = func->push(ict::Block::create()).ptr();
        bool hadSomething = false;

        auto bracket = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (bracket.type != '{')
            throw misc::SourceError(bracket, "Expected `{` to open function impl");

        BlockNameTable bnt;
        VRegNameTable vrnt;

        std::unordered_map<ict::Op*, View> args;

        while (1) {
            auto tok = misc::tokenize(source, misc::TOKF_DOTNAME);
            if (tok.type == misc::TOK_EOF)
                throw misc::SourceError(bracket, "Not closed");
            if (tok.type == '}') 
                break;
            if (tok.type != misc::TOK_NAME)
                throw misc::SourceError(tok, "Label name, operation name or vreg name expected here");

            if (tok.view[0] == '%') {
                auto second = misc::tokenize(source, misc::TOKF_DOTNAME);
                if (second.type == ':') {
                    if (hadSomething)
                        block = func->push(ict::Block::create()).ptr();
                    block->setName(tok.view.substr(1));
                    misc::verb(TAG) << "Have block " << block->name() << "\n";
                    bnt[block->name()] = block;
                } else if (second.type == misc::TOK_NAME && isalpha(second.view[0])) {
                    auto op = block->push(parseOp(mgr, second, source)).ptr();
                    op->setVregName(tok.view.substr(1));
                    vrnt[op->vregName()] = op;
                    args[op] = getArgsView(mgr, bracket, source);
                } else {
                    throw misc::SourceError(second, "Expected `:` (for block label) or operator name here");
                }
            } else if (isalpha(tok.view[0])) {
                auto op = block->push(parseOp(mgr, tok, source));
                args[op] = getArgsView(mgr, bracket, source);
            } else {
                throw misc::SourceError(tok, "Label name, operation name or vreg name expected here");
            }
            hadSomething = true;
        }

        for (auto &block : *func)
            for (auto &op : *block)
                parseOpArgs(mgr, op.get(), args[op.get()], gnt, bnt, vrnt);

        return func;
    }

    misc::UPtr<ict::Module> compile(ict::Manager *mgr) const override {
        misc::info(TAG) << "Parsing " << misc::ACCENT << mgr->filename() << misc::RST << '\n';
        View source = mgr->source();
        auto mod = ict::Module::create();
        GlobalNameTable gnt;
        try {
            while (1) {
                misc::Token tok = misc::tokenize(source, misc::TOKF_DOTNAME);
                if (tok.type == misc::TOK_EOF)
                    break;
                if (tok.type != misc::TOK_NAME)
                    throw misc::SourceError(tok, "Expected global type");
                if (tok.view == "func_impl")
                    mod->push(parseFunction(mgr, source, gnt));
            }
        } catch (misc::SourceError &e) {
            auto msg = misc::error(TAG);
            e.writeFormatted(msg.stream(), mgr->filename(), mgr->source());
            return nullptr;
        }
        return mod;
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new IrFrontend();
}
