#include "ict/ir.hpp"
#include "ict/mod.hpp"
#include "misclib/defs.hpp"
#include "misclib/dump_stream.hpp"
#include "modlib_manager.hpp"
#include "ict/arch/ivm.hpp"
#include <iomanip>
#include <ostream>

#define TAG "ict.ivm.emit"

using namespace ict::ar::ivm;
using namespace misc::color;
using misc::View;

class IvmBackend : public ict::Backend {
    View id() const override { return "ict.ivm.backend"; }
    View brief() const override { return "ICT backend for IVM"; }
    misc::View archName() const override { return "ivm"; }

    int str2lowOp(View name) const override {
#define X(id, inv, from, iname, textname, instr) if (textname == name) return id;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X
        return ict::OP_UNKNOWN_OP;
    }

    View lowOp2str(int k) const override {
#define X(id, inv, from, name, textname, instr) if (k == name) return textname;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X

        return "(invalid op)";
    }

    View m_instrName(int k) const
    {
#define X(id, inv, from, name, textname, instr) if (k == name && instr != nullptr) return instr;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X
        return "";
    }

    void emitFunction(std::ostream &output, ict::FunctionImpl *func) const {
        output << DGRAY << "///\n" << "/// " << RST << *func->decl() << DGRAY << "///\n" << RST;
        output << ".func\n";
        output << func->decl()->name() << ":\n" << misc::beginBlock;
        for (auto block : func->blocks()) {
            output << "_" << func->decl()->name() << "." << block << ":\n" << misc::beginBlock;
            for (auto op : block->operations()) {
                if (m_instrName(op->kind()).empty()) {
                    misc::error(TAG) << "Op " << op->name() << " is not IVM instruction";
                    abort();
                }
                output << m_instrName(op->kind());
                for (auto arg : op->args()) {
                    if (auto varg = dynamic_cast<ict::VRegArg*>(arg)) {
                        if (dynamic_cast<const StackType*>(varg->ptr()->returnType().get()) != nullptr)
                            continue; // used to monitor values on stack, ignored by backend
                        else
                            misc::error(TAG) << "Vreg register " << *arg << " made it to backend";

                    } else if (auto iarg = dynamic_cast<ict::ConstArg*>(arg))
                        output << " " << iarg->value();
                    else if (auto barg = dynamic_cast<ict::BlockArg*>(arg))
                        output << " _" << func->decl()->name() << "." << barg->ptr();
                    else if (auto farg = dynamic_cast<ict::GlobalArg*>(arg))
                        output << " " << farg->ptr()->name();
                    else {
                        misc::error(TAG) << "Arg " << *arg << " cannot be emitted";
                        abort();
                    }
                }
                output << "\n";
            }
            output << misc::endBlock;
        }
        output << misc::endBlock << "\n";
    }

    void emitBss(std::ostream &out, ict::BssImpl *bss, size_t &counter) const
    {
        out << DGRAY << "///\n" << "/// " << RST << *bss << DGRAY << "///\n" << RST;
        out << ".const " << bss->decl()->name() << " " << counter << "\n";
        counter += bss->size();
        out << "\n";
    }

    void emitBlob(std::ostream &out, ict::BlobImpl *blob) const
    {
        out << DGRAY << "///\n" << "/// " << RST << *blob->decl() << DGRAY << "///\n" << RST;
        out << blob->decl()->name() << ":" << misc::beginBlock;
        std::ios save(nullptr);
        save.copyfmt(out);
        for (size_t i = 0; i < blob->blob().size(); ++i) {
            if (i % 16 == 0) out << "\n.ascii \"";
            out << "\\x" << std::setw(2) << std::hex << std::setfill('0') << (int) blob->blob()[i];
            if (i % 16 == 15 || i == blob->blob().size() - 1) out << "\"";
        }
        out.copyfmt(save);
        out << std::setfill(' ') << misc::endBlock << "\n";
    }

    void emit(ict::Manager *mgr, std::ostream &output) const override
    {
        size_t bss_idx = 0x1000; // rom begin
        for (auto it : mgr->module()->impls()) {
            if (auto func = dynamic_cast<ict::FunctionImpl*>(it)) {
                emitFunction(output, func);
            } else if (auto bss = dynamic_cast<ict::BssImpl*>(it)) {
                emitBss(output, bss, bss_idx);
            } else if (auto blob = dynamic_cast<ict::BlobImpl*>(it)) {
                emitBlob(output, blob);
            }
        }
        output << ".const _ict_bss_end " << bss_idx << "\n";
    }
    
    bool lowOpHasSideEffects(int k) const override {
        switch (k) {
            case IVM_R_CALL:
            case IVM_R_SFLOAD8: case IVM_R_SFLOAD16: case IVM_R_SFLOAD32: case IVM_R_SFLOAD64:
            case IVM_R_SFSTORE8: case IVM_R_SFSTORE16: case IVM_R_SFSTORE32: case IVM_R_SFSTORE64:
            case IVM_R_PUT8: case IVM_R_PUT16: case IVM_R_PUT32: case IVM_R_PUT64:
            case IVM_R_GET8: case IVM_R_GET16: case IVM_R_GET32: case IVM_R_GET64:
                return true;
            default:
                return false;
        }
    }

    bool lowOpRequiresType(int k) const override {
        return false;
    }
    
    ict::UPtr<ict::Type> createReturnType(ict::Operation *op) const override {
        if (op->kind() == IVM_FROMSTACK)
            return op->tparam() ? op->tparam()->clone() : ict::Type::i64_t();
        else
            return StackType::create();
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new IvmBackend();
}
