#include "ict/ir.hpp"
#include "ict/mod.hpp"
#include "misclib/array_view.hpp"
#include "misclib/defs.hpp"
#include "misclib/dump_stream.hpp"
#include "modlib_manager.hpp"
#include <initializer_list>
#include <unordered_map>
#include "ict/arch/ivm.hpp"

#define TAG "ict.ivm"

using namespace misc::color;
using namespace ict::ar::ivm;
using misc::View;

struct FunctionMaker {
    struct BlockInfo {
        size_t stackSizeAfter;
    };

    std::ostream &os;
    ict::Function *func;
    std::unordered_map<const ict::Block*, BlockInfo> writtenBlocks;
    std::unordered_map<const ict::Op*, size_t> placeOnStack;

    FunctionMaker(std::ostream &os, ict::Function *func)
        :os(os), func(func) {}

    void instr(View name) { os << BLUE << name << RST << "\n"; }
    void instr(View name, ict::Integer val) { os << BLUE << name << " " << YELLOW << val << RST << "\n"; }
    void instr(View name, ict::Block *block) { os << BLUE << name << " " << GREEN << "_" << block->parent()->name() << "." << block->slotInParent() << RST << "\n"; }
    void instr(View name, View arg) { os << BLUE << name << " " << RED << arg << RST << "\n"; }

    bool willBeUsedAfter(const ict::Op *value, const ict::Op *use) {
        return value->refs().size() > 1;
    }

    void opArgs(const ict::Op *forUseBy, size_t &stackTop, misc::ConstArrayView<const ict::VRegArg *> ops) {
        // If we do not use some prefix afterwards, and
        // it is exactly on top of the stack, use it:
        //
        //      stack       : %c ... %a %b
        //      operation   : Op %a %b %c
        //      result      : ... generated code for getting %c on top,
        //                        %a and %b will be used
        size_t inPlace = 0;

        size_t prefixSize = stackTop - placeOnStack[ops[0]->ptr()];
        if (prefixSize > ops.size())
            goto generatePulls;
        for (size_t i = 0; i < ops.size(); ++i)
            if (willBeUsedAfter(ops[i]->ptr(), forUseBy) || placeOnStack[ops[i]->ptr()] != stackTop - ops.size() + i)
                goto generatePulls;

        inPlace = prefixSize;

    generatePulls:
        for (size_t i = inPlace; i < ops.size(); ++i) {
            instr("pull", placeOnStack[ops[i]->ptr()] - stackTop + 1);
            stackTop++;
        }
    }

    void opAftermath(const ict::Op *by, size_t &stackTop, bool pushedResult, size_t argsEaten) {
        stackTop -= argsEaten;
        if (pushedResult) {
            placeOnStack[by] = stackTop;
            ++stackTop;
        }
    }

    void writeOp(ict::Op *op, size_t &stackTop) {
        switch (op->kind()) {

            case ict::CONST:
                instr("push", op->carg(0)->value());
                opAftermath(op, stackTop, true, 0);
                break;

            case ict::ADD:
            case ict::SUB:
            case ict::DIV:
            case ict::MUL:
            case ict::MOD: {
                opArgs(op, stackTop, { op->varg(0), op->varg(1) });
                switch (op->kind()) {
                    case ict::ADD: instr("add"); break;
                    case ict::SUB: instr("sub"); break;
                    case ict::DIV: instr("div"); break;
                    case ict::MUL: instr("mul"); break;
                    case ict::MOD: instr("mod"); break;
                    default: assert(false);
                }
                opAftermath(op, stackTop, true, 2);
                break;
            }

            case ict::LT:
            case ict::GT:
            case ict::GE:
            case ict::LE:
            case ict::EQ:
            case ict::NEQ: {
                // For some reason a year ago I decided on inverse order
                opArgs(op, stackTop, { op->varg(1), op->varg(0) });
                switch (op->kind()) {
                    case ict::LT: instr("lt"); break;
                    case ict::GT: instr("gt"); break;
                    case ict::GE: instr("ge"); break;
                    case ict::LE: instr("le"); break;
                    case ict::EQ: instr("eq"); break;
                    case ict::NEQ: instr("eq"); instr("iszero"); break;
                    default: assert(false);
                }
                opAftermath(op, stackTop, true, 2);
                break;
            }

            case ict::DEBUG_PRINT: {
                for (auto &arg : op->children()) {
                    if (auto vreg = dynamic_cast<ict::VRegArg*>(arg.get())) {
                        opArgs(op, stackTop, { vreg });
                        instr("call", "__ict_debug_print"); 
                        opAftermath(op, stackTop, false, 1);
                    } else if (auto num = dynamic_cast<ict::ConstArg*>(arg.get())) {
                        instr("push", num->value());
                        instr("call", "__ict_debug_print");
                    }
                }
                break;
            }

            case ict::RET: {
                instr("dropn", stackTop);
                instr("sfend");
                instr("spop");
                instr("ret");
                break;
            };

            case ict::BR: {
                auto block = op->barg(0);
                instr("jmp", block->ptr());
                os << "\n";
                break;
            }

            case ict::BC: {
                auto cond = op->varg(0);
                auto then = op->barg(1);
                auto otherwise = op->barg(2);
                opArgs(op, stackTop, { cond });
                instr("jnz", then->ptr());
                instr("jmp", otherwise->ptr());
                opAftermath(op, stackTop, false, 1);
                break;
            }

            case ict::ALLOCA: {
                misc::warn(TAG) << func->name() << ": ict::ALLOCA should be removed by some stack planner";
                // Just get new slot each time, not reusing old slots
                instr("spush");
                instr("gsp");
                opAftermath(op, stackTop, false, 1);
                break;
            }

            case ict::LOAD: {
                opArgs(op, stackTop, { op->varg(0) });
                instr("get64");
                opAftermath(op, stackTop, true, 1);
                break;
            }

            case ict::STORE: {
                auto dest = op->varg(0);
                auto val = op->varg(1);
                opArgs(op, stackTop, { val, dest });
                instr("put64");
                opAftermath(op, stackTop, false, 2);
                break;
            }

            default: {
                os << RED << "// TODO: Unknown operation: " << op->kind().name() << RST << "\n";
                break;
            }
        }
    }

    BlockInfo &writeBlock(ict::Block *block) {
        if (writtenBlocks.count(block))
            return writtenBlocks[block];
        
        os << PURPLE << "_" << func->name() << "." << block->slotInParent() << ":";
        if (!block->name().empty())
            os << DGRAY << " // %" << block->name();
        os << RST << "\n" << misc::beginBlock;

        size_t stackPos = 0;

        for (auto &i : *block) {
            os << DGRAY << "// " << *i << RST;
            writeOp(i.get(), stackPos);
        }
        os << misc::endBlock;
        
        return writtenBlocks[block] = {
            .stackSizeAfter = stackPos
        };
    }

    void run() {
        os << RED << func->name() << ":\n" << RST << misc::beginBlock;
        instr("spush");
        instr("sfbegin");
        os << misc::endBlock;
        for (auto &i : *func)
            writeBlock(i.get());
    }
};

class IvmBackend : public ict::Backend {
    View id() const override { return "ict.ivm.backend"; }
    View brief() const override { return "ICT backend for IVM"; }
    misc::View archName() const override { return "ivm"; }

    ict::OpKind str2lowOp(View name) const override { return ict::UNKNOWN_OP; }
    View lowOp2str(ict::OpKind k) const override {
#define X(id, from, name, textname, instr) if (k == name) return textname;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X

        return "(unknownn)";
    }

    void emit(ict::Manager *mgr, std::ostream &output) const override {
        for (auto &i : *mgr->module()) {
            FunctionMaker fm(output, i.get());
            fm.run();
        }
    }

    bool lowOpReturns(ict::OpKind k) const override {
        return k == IVM_FROMSTACK;
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new IvmBackend();
}
