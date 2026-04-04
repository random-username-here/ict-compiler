/**
 * \file
 * \brief High level IR to low level IR conversion pass for IVM
 */
#include "ict/mod.hpp"
#include "ict/arch/ivm.hpp"
#include "ict/ir.hpp"
#include "misclib/dump_stream.hpp"
using namespace ict;
using namespace ict::ar::ivm;
using namespace misc::color;

#define TAG "ict.ivm.hl2ll"

class HL2LL : public Pass
{
    int order() const override { return 0; };
    std::string_view id() const override { return "ict.ivm.hl2ll"; }
    std::string_view brief() const override { return "ICT IVM pass to convert high-level IR to low-level"; }

    OpKind m_getOneToOneMapping(OpKind kind) const
    {
#define X(id, inv, from, name, textname, instr) if (kind == from) return name;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X
        return 0;
    }

    bool m_hasInvertedOperandOrder(OpKind kind) const
    {
#define X(id, inv, from, name, textname, instr) if (kind == from) return inv;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X
        return false;
    }

    /** Operator is 1-to-1 mapped from high-level IR */
    void m_replaceWithOneToOneMapping(Op *op, OpKind kind) const
    {
        auto inserter = BlockBuilder::before(op);
        bool inverse = m_hasInvertedOperandOrder(op->kind());
        for (size_t i = (inverse ? op->size()-1 : 0);
             inverse ? (i != (size_t) -1) : (i < op->size());
             inverse ? --i : ++i
        ) {
            auto arg = op->child(i).ptr();
            if (auto iarg = dynamic_cast<ConstArg*>(arg))
                inserter.create(IVM_R_PUSH, iarg->value());
            else if (auto varg = dynamic_cast<VRegArg*>(arg))
                inserter.create(IVM_TOSTACK, varg->ptr());
            else {
                misc::error(TAG) << "Cannot comprehend arg " << *arg << " for one-to-one mapping";
                abort();
            }
        }
        inserter.create(kind);
        if (op->kind().doesReturn()) {
            auto load = inserter.create(IVM_FROMSTACK);
            op->replaceRefsWith(load);
        }
        op->extractSelf();
    }

    void m_convertDebugPrint(Op *op) const
    {
        auto ins = BlockBuilder::before(op);
        for (auto &arg : op->children()) {
            if (auto iarg = dynamic_cast<ConstArg*>(arg.get()))
                ins.create(IVM_R_PUSH, iarg->value());
            else if (auto varg = dynamic_cast<VRegArg*>(arg.get()))
                ins.create(IVM_TOSTACK, varg->ptr());
            else {
                misc::error(TAG) << "DebugPrint has unexpected arg " << *arg;
                abort();
            }
            ins.create(IVM_DEBUG_PRINT_INT);
        }
        ins.create(IVM_R_PUSH, (Integer) '\n');
        ins.create(IVM_DEBUG_PRINT_CHAR);
        op->extractSelf();
    }

    void m_convert(Op *op) const
    {
        OpKind o2o = m_getOneToOneMapping(op->kind());
        if (o2o) {
            m_replaceWithOneToOneMapping(op, o2o);
            return;
        }

        auto inserter = BlockBuilder::before(op);
        switch(op->kind()) {
            case CONST: {
                inserter.create(IVM_R_PUSH, op->carg(0)->value());
                auto val = inserter.create(IVM_FROMSTACK);
                op->replaceRefsWith(val);
                op->extractSelf();
                return;
            }
            case RET:
                // TODO: returning something
                inserter.create(IVM_R_SFEND);
                inserter.create(IVM_R_SPOP);
                inserter.create(IVM_R_RET);
                op->extractSelf();
                return;
            case BR:
                inserter.create(IVM_R_RJMP, op->barg(0)->ptr());
                op->extractSelf();
                return;
            case BC:
                inserter.create(IVM_TOSTACK, op->varg(0)->ptr());
                inserter.create(IVM_R_RJNZ, op->barg(1)->ptr());
                inserter.create(IVM_R_RJMP, op->barg(2)->ptr());
                op->extractSelf();
                return;
            case ALLOCA: {
                inserter.create(IVM_R_SPUSH, 0);
                inserter.create(IVM_R_GSP);
                auto res = inserter.create(IVM_FROMSTACK);
                for (auto i : op->refs())
                    misc::verb(TAG) << i << " " << i->parent();
                op->replaceRefsWith(res);
                op->extractSelf();
                return;
            }
            case DEBUG_PRINT:
                m_convertDebugPrint(op);
                return;
            default:
                misc::error(TAG) << "Cannot convert op " << ACCENT << op->kind().name() << RST;
                abort();
        }
    }

    void run(Manager *mgr) const override
    {
        if (mgr->backend()->archName() != "ivm") {
            misc::info(TAG) << "Backend is not set to `ivm`, skipping";
            return;
        }

        misc::info(TAG) << "Work is being done";
        for (auto &func : mgr->module()->children()) {
            for (auto &block : func->children()) {
                // We will be changing array, iterate backwards
                for (size_t i = block->size()-1; i != (size_t) -1; --i)
                    m_convert(block->child(i).ptr());
            }
            auto ins = BlockBuilder(func->child(0), 0);
            ins.create(IVM_R_SPUSH);
            ins.create(IVM_R_SFBEGIN);
        }
        misc::verb(TAG) << "Resulting IR:\n" << misc::beginBlock << *mgr->module() << misc::endBlock;

    }
};

extern "C" Mod *modlib_create(ModManager *mm)
{
    return new HL2LL();
}
