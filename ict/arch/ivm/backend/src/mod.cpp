#include "ict/ir.hpp"
#include "ict/mod.hpp"
#include "misclib/defs.hpp"
#include "misclib/dump_stream.hpp"
#include "modlib_manager.hpp"
#include "ict/arch/ivm.hpp"

#define TAG "ict.ivm.emit"

using namespace ict::ar::ivm;
using misc::View;

class IvmBackend : public ict::Backend {
    View id() const override { return "ict.ivm.backend"; }
    View brief() const override { return "ICT backend for IVM"; }
    misc::View archName() const override { return "ivm"; }

    ict::OpKind str2lowOp(View name) const override { return ict::UNKNOWN_OP; }
    View lowOp2str(ict::OpKind k) const override {
#define X(id, inv, from, name, textname, instr) if (k == name) return textname;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X

        return "(invalid op)";
    }

    bool lowOpReturns(ict::OpKind k) const override { return k == IVM_FROMSTACK; }

    View m_instrName(ict::OpKind k) const
    {
#define X(id, inv, from, name, textname, instr) if (k == name && instr != nullptr) return instr;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X
        return "";
    }

    void emit(ict::Manager *mgr, std::ostream &output) const override
    {
        for (auto &func : mgr->module()->children()) {
            output << ".func\n";
            output << func->name() << ":\n" << misc::beginBlock;
            for (auto &block : func->children()) {
                output << "_" << func->name() << "." << block->slotInParent() << ":\n" << misc::beginBlock;
                for (auto &op : block->children()) {
                    if (m_instrName(op->kind()).empty()) {
                        misc::error(TAG) << "Op " << op->kind().name(mgr) << " is not IVM instruction";
                        abort();
                    }
                    output << m_instrName(op->kind());
                    for (auto &arg : op->children()) {
                        if (auto iarg = dynamic_cast<ict::ConstArg*>(arg.get()))
                            output << " " << iarg->value();
                        else if (auto barg = dynamic_cast<ict::BlockArg*>(arg.get()))
                            output << " _" << func->name() << "." << barg->ptr()->slotInParent();
                        else {
                            misc::error(TAG) << "Arg " << *arg << " cannot be emitted";
                            abort();
                        }
                    }
                    output << "\n";
                }
                output << misc::endBlock;
            }
            output << misc::endBlock;
        }
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new IvmBackend();
}
