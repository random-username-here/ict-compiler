/**
 * \file
 * \brief ICT frontend for IR files
 * \todo Allow backward references
 */
#include "ict/mod.hpp"
#include "ict/ir.hpp"
#include "misclib/defs.hpp"
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

    using GlobalNameTable = std::unordered_map<View, ict::FunctionDecl*>;
    using BlockNameTable = std::unordered_map<View, ict::BasicBlock*>;
    using VRegNameTable = std::unordered_map<View, ict::Operation*>;

    UPtr<ict::Type> parseType(ict::Module *mod, View &source) const {
        auto name = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (name.type != misc::TOK_NAME)
            throw misc::SourceError(name, "Expected a type name");
        if (name.view == "void") return ict::Type::void_t();
        if (name.view == "i64") return ict::Type::i64_t();
        if (name.view == "i32") return ict::Type::i32_t();
        if (name.view == "i16") return ict::Type::i16_t();
        if (name.view == "i8") return ict::Type::i8_t();
        if (name.view == "ptr") return ict::Type::ptr_t();
        throw misc::SourceError(name, "This is not a valid type name");
    }

    UPtr<ict::Operation> parseOpNameAndType(
            ict::Module *mod,
            misc::Token name, View &source
    ) const {
        misc::verb(TAG) << "    Found operation " << misc::ACCENT << name.view << misc::RST << "\n";

        int kind = ict::Operation::kindFromString(ict::Manager::main(), name.view);
        if (kind == ict::OP_UNKNOWN_OP)
            throw misc::SourceError(name, "Operation name is not known in high-level IR or in low-level IR");

        misc::Token punct;
        View v = misc::tokenize(source, misc::TOKF_DOTNAME, &punct);
        UPtr<ict::Type> type = nullptr;
        if (punct.type == '[') {
            // a type!
            source = v;
            type = parseType(mod, source);
            punct = misc::tokenize(source, misc::TOKF_DOTNAME);
            if (punct.type != ']')
                throw misc::SourceError(punct, "Expected `]` to close operation type");
        }

        auto op = ict::Operation::create();
        op->setKind(kind);
        op->tparam() = std::move(type);
        return op;
    }

    View getArgsView(misc::Token blockBrace, View &source) const {
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
            ict::Operation *op,
            View source,
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
                op->args().push(ict::ConstArg::create(val));
            } else if (tok.type == misc::TOK_NAME) {
                if (tok.view[0] == '@') {
                    auto func = op->parent()->parent()->parent()->findFunc(tok.view.substr(1));
                    if (!func)
                        throw misc::SourceError(tok, "Unknown function");
                    op->args().push(ict::FuncArg::create(func));
                } else if (tok.view[0] == '%') {
                    auto name = tok.view.substr(1);
                    if (bnt.count(name)) {
                        op->args().push(ict::BlockArg::create(bnt.at(name)));
                    } else if (vrnt.count(name)) {
                        op->args().push(ict::VRegArg::create(vrnt.at(name)));
                    } else {
                        auto decl = op->parent()->parent()->decl();
                        ict::ArgDecl *argDecl = nullptr;
                        for (auto arg : decl->args()) 
                            if (arg->name() == name)
                                argDecl = arg;
                        if (decl)
                            op->args().push(ict::ArgArg::create(argDecl));
                        else
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

    void implementFunction(ict::Module *mod, View &source) const {
        auto name = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (name.type != misc::TOK_NAME || name.view[0] != '@')
            throw misc::SourceError(name, "Expected a global function name to implement");
        misc::info(TAG) << "Reading function " << misc::ACCENT << name.view << misc::RST << '\n';

        auto decl = mod->findFunc(name.view.substr(1));
        if (!decl)
            throw misc::SourceError(name, "Function not declred before implementation");
        if (decl->impl())
            throw misc::SourceError(name, "This is a second implementation of this function");

        auto bracket = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (bracket.type != '{')
            throw misc::SourceError(bracket, "Expected `{` to open function impl");

        auto func = decl->implement();
        auto block = func->createBlock("");
        bool hadSomething = false;

        BlockNameTable bnt;
        VRegNameTable vrnt;

        std::unordered_map<ict::Operation*, View> args;

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
                    // block label `%block:`
                    if (hadSomething)
                        block = func->createBlock("");
                    block->setName(tok.view.substr(1));
                    bnt[block->name()] = block;
                } else if (second.type == misc::TOK_NAME && isalpha(second.view[0])) {
                    // operation which returns `%res Add ...`
                    auto op = block->operations().insertBefore(nullptr, parseOpNameAndType(mod, second, source));
                    op->setVregName(tok.view.substr(1));
                    vrnt[op->vregName()] = op;
                    args[op] = getArgsView(bracket, source);
                } else {
                    throw misc::SourceError(second, "Expected `:` (for block label) or operator name here");
                }
            } else if (isalpha(tok.view[0])) {
                // operation with no return
                auto op = block->operations().insertBefore(nullptr, parseOpNameAndType(mod, tok, source));
                args[op] = getArgsView(bracket, source);
            } else {
                throw misc::SourceError(tok, "Label name, operation name or vreg name expected here");
            }
            hadSomething = true;
        }

        misc::info(TAG) << "Populating function " << misc::ACCENT << name.view << misc::RST << "'s operation arguments" << '\n';
        for (auto block : func->blocks()) {
            for (auto op : block->operations()) {
                parseOpArgs(op, args[op], bnt, vrnt);
                if (!op->verifyArgsOnly())
                    throw misc::SourceError(name, "Operation with invalid arguments"); // TODO: better errors
                op->genReturnType();
            }
        }
    }


    void declareFunction(ict::Module *mod, View &source) const {
        auto name = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (name.type != misc::TOK_NAME || name.view[0] != '@')
            throw misc::SourceError(name, "Expected a global function name");
        auto punct = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (punct.type != '(')
            throw misc::SourceError(punct, "Expected `(` after function name");

        auto decl = ict::FunctionDecl::create();
        decl->setName(name.view.substr(1));

        while (punct.type != ')') {
            std::string name = "";
            misc::Token maybeName;
            View ns = misc::tokenize(source, misc::TOKF_DOTNAME, &maybeName);
            if (maybeName.type == misc::TOK_NAME && maybeName.view[0] == '%') {
                name = maybeName.view.substr(1);
                source = ns;
            } else if (maybeName.type == ')') {
                source = ns;
                break;
            }
            decl->args().push(ict::ArgDecl::create(parseType(mod, source), name));
            punct = misc::tokenize(source, misc::TOKF_DOTNAME);
            if (punct.type == misc::TOK_EOF)
                throw misc::SourceError(punct, "Argument list not closed");
            else if (punct.type != ',' && punct.type != ')')
                throw misc::SourceError(punct, "Expected `,` or `)`");
        }

        decl->retType().replace(parseType(mod, source));
        punct = misc::tokenize(source, misc::TOKF_DOTNAME);
        if (punct.type != ';')
            throw misc::SourceError(punct, "Expected `;` after function decl");

        mod->decls().insertBefore(nullptr, std::move(decl));
    }

    bool compile(ict::Manager *mgr, ict::Module *mod) const override {
        misc::info(TAG) << "Parsing " << misc::ACCENT << mgr->filename() << misc::RST << '\n';
        View source = mgr->source();
        GlobalNameTable gnt;
        try {
            while (1) {
                misc::Token tok = misc::tokenize(source, misc::TOKF_DOTNAME);
                if (tok.type == misc::TOK_EOF)
                    break;
                if (tok.type != misc::TOK_NAME)
                    throw misc::SourceError(tok, "Expected a directive");
                if (tok.view == "func_decl")
                    declareFunction(mod, source);
                else if (tok.view == "func_impl")
                    implementFunction(mod, source);
                else
                    throw misc::SourceError(tok, "Unknown directive, expected `func_impl` or `func_decl`.");
            }
        } catch (misc::SourceError &e) {
            auto msg = misc::error(TAG);
            e.writeFormatted(msg.stream(), mgr->filename(), mgr->source());
            return false;
        }

        misc::info(TAG) << "Verifying " << misc::ACCENT << mgr->filename() << misc::RST << '\n';

        if (mod->verify()) {
            misc::info(TAG) << misc::ACCENT << mgr->filename() << misc::RST << " is good\n";
            return true;
        } else {
            misc::error(TAG) << misc::ACCENT << mgr->filename() << misc::RST << " has problems, bailing out\n";
            return false;
        }
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new IrFrontend();
}
