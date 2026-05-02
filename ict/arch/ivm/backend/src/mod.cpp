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

    void emit(ict::Manager *mgr, std::ostream &output) const override
    {
        for (auto it : mgr->module()->impls()) {
            auto func = dynamic_cast<ict::FunctionImpl*>(it);
            if (!func) continue; // TODO: blobs
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
                        if (auto iarg = dynamic_cast<ict::ConstArg*>(arg))
                            output << " " << iarg->value();
                        else if (auto barg = dynamic_cast<ict::BlockArg*>(arg))
                            output << " _" << func->decl()->name() << "." << barg->ptr();
                        else if (auto farg = dynamic_cast<ict::FuncArg*>(arg))
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
            output << misc::endBlock;
        }
    }

    bool lowOpRequiresType(int k) const override {
        return false;
    }
    
    ict::UPtr<ict::Type> createReturnType(ict::Operation *op) const override {
        if (op->kind() == IVM_FROMSTACK)
            return op->tparam() ? op->tparam()->clone() : ict::Type::i64_t();
        else
            return ict::Type::void_t();
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new IvmBackend();
}
