/**
 * \file
 * \brief High level IR to low level IR conversion pass for IVM
 * \todo refer to FromStack/ToStack in operations, so optimizers would not need to search for them
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

    int m_getOneToOneMapping(int kind) const
    {
#define X(id, inv, from, name, textname, instr) if (kind == from) return name;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X
        return 0;
    }

    bool m_hasInvertedOperandOrder(int kind) const
    {
#define X(id, inv, from, name, textname, instr) if (kind == from) return inv;
        ICT_IVM_FOR_ALL_OPS(X)
#undef X
        return false;
    }

    /** Operator is 1-to-1 mapped from high-level IR */
    void m_replaceWithOneToOneMapping(Operation *op, int mlirOpKind) const
    {
        auto inserter = op->parentList()->inserterBefore(op);
        bool inverse = m_hasInvertedOperandOrder(op->kind());
        for (size_t i = (inverse ? op->args().size()-1 : 0);
             inverse ? (i != (size_t) -1) : (i < op->args().size());
             inverse ? --i : ++i
        ) {
            auto arg = op->arg(i);
            if (auto iarg = dynamic_cast<ConstArg*>(arg))
                inserter.create(IVM_R_PUSH, nullptr, iarg->value());
            else if (auto varg = dynamic_cast<VRegArg*>(arg))
                inserter.create(IVM_TOSTACK, varg->ptr()->returnType()->clone(), varg->ptr());
            else {
                misc::error(TAG) << "Cannot comprehend arg " << *arg << " for one-to-one mapping";
                abort();
            }
        }
        inserter.create(mlirOpKind);
        if (!op->returnType()->isVoid()) {
            auto load = inserter.create(IVM_FROMSTACK, op->returnType()->clone());
            op->replaceRefsWith(load);
        }
        op->extractSelf();
    }

    void m_convertDebugPrint(Operation *op) const
    {
        auto ins = op->parentList()->inserterBefore(op);
        for (auto arg : op->args()) {
            if (auto iarg = dynamic_cast<ConstArg*>(arg))
                ins.create(IVM_R_PUSH, nullptr, iarg->value());
            else if (auto varg = dynamic_cast<VRegArg*>(arg))
                ins.create(IVM_TOSTACK, nullptr, varg->ptr());
            else {
                misc::error(TAG) << "DebugPrint has unexpected arg " << *arg;
                abort();
            }
            ins.create(IVM_DEBUG_PRINT_INT, nullptr);
        }
        ins.create(IVM_R_PUSH, nullptr, (Integer) '\n');
        ins.create(IVM_DEBUG_PRINT_CHAR, nullptr);
        op->extractSelf();
    }

    void m_convert(Operation *op, std::unordered_map<ArgDecl*, int> argOffsets) const
    {
        int o2o = m_getOneToOneMapping(op->kind());
        if (o2o) {
            m_replaceWithOneToOneMapping(op, o2o);
            return;
        }

        auto inserter = op->parentList()->inserterBefore(op);
        switch(op->kind()) {
            case OP_CONST: {
                inserter.create(IVM_R_PUSH, nullptr, op->arg_c(0)->value());
                auto val = inserter.create(IVM_FROMSTACK, nullptr);
                op->replaceRefsWith(val);
                op->extractSelf();
                return;
            }
            case OP_RET: {
                // TODO: returning something
                if (op->args().size()) {
                    inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr());
                    inserter.create(IVM_R_SFEND, nullptr);
                    inserter.create(IVM_R_SPOP, nullptr);
                    inserter.create(IVM_R_SWAP, nullptr);
                    inserter.create(IVM_R_SPUSH, nullptr);
                    inserter.create(IVM_R_RET, nullptr);
                } else {
                    inserter.create(IVM_R_SFEND, nullptr);
                    inserter.create(IVM_R_SPOP, nullptr);
                    inserter.create(IVM_R_RET, nullptr);
                }
                op->extractSelf();
                return;
            }
            case OP_BR:
                inserter.create(IVM_R_RJMP, nullptr, op->arg_b(0)->ptr());
                op->extractSelf();
                return;
            case OP_BC:
                inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr());
                inserter.create(IVM_R_RJNZ, nullptr, op->arg_b(1)->ptr());
                inserter.create(IVM_R_RJMP, nullptr, op->arg_b(2)->ptr());
                op->extractSelf();
                return;
            case OP_ALLOCA: {
                auto delta = -op->tparam()->size();
                auto n = inserter.create(IVM_R_PUSH, nullptr, delta);
                inserter.create(IVM_R_SADD, nullptr);
                inserter.create(IVM_R_GSP, nullptr);
                auto res = inserter.create(IVM_FROMSTACK, nullptr);
                for (auto i : op->refs())
                    misc::verb(TAG) << i << " " << i->parent();
                op->replaceRefsWith(res);
                op->extractSelf();
                return;
            }
            case OP_STORE: {
                inserter.create(IVM_TOSTACK, nullptr, op->arg_v(1)->ptr()); // val
                inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr()); // pos
                assert(op->tparam()->isSimple());
                switch (op->tparam()->size()) {
                    case 1: inserter.create(IVM_R_PUT8, nullptr); break;
                    case 2: inserter.create(IVM_R_PUT16, nullptr); break;
                    case 4: inserter.create(IVM_R_PUT32, nullptr); break;
                    case 8: inserter.create(IVM_R_PUT64, nullptr); break;
                    default: assert(false);
                }
                op->extractSelf();
                return;
            };

            case OP_LOAD: {
                inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr()); // pos
                assert(op->returnType()->isSimple());
                switch (op->returnType()->size()) {
                    case 1: inserter.create(IVM_R_GET8, nullptr); break;
                    case 2: inserter.create(IVM_R_GET16, nullptr); break;
                    case 4: inserter.create(IVM_R_GET32, nullptr); break;
                    case 8: inserter.create(IVM_R_GET64, nullptr); break;
                    default: assert(false);
                }
                auto g = inserter.create(IVM_FROMSTACK, op->returnType()->clone());
                op->replaceRefsWith(g);
                op->extractSelf();
                return;
            };

            case OP_ARGPTR: {
                inserter.create(IVM_R_GSF, nullptr);
                inserter.create(IVM_R_PUSH, nullptr, argOffsets[op->arg_a(0)->ptr()]);
                inserter.create(IVM_R_ADD, nullptr);
                auto g = inserter.create(IVM_FROMSTACK, Type::ptr_t());
                op->replaceRefsWith(g);
                op->extractSelf();
                return;
            };

            case OP_CALL: {
                auto func = op->arg_f(0);
                for (size_t i = op->args().size()-1; i != 0; --i) {
                    inserter.create(IVM_TOSTACK, nullptr, op->arg_v(i)->ptr());
                    inserter.create(IVM_R_SPUSH, nullptr);
                }
                inserter.create(IVM_R_CALL, nullptr, func->ptr());
                if (!func->ptr()->retType()->isVoid()) {
                    inserter.create(IVM_R_SPOP, nullptr);
                    auto g = inserter.create(IVM_FROMSTACK, op->returnType()->clone());
                    op->replaceRefsWith(g);
                }
                inserter.create(IVM_R_SADD, nullptr, 8 * (op->args().size() - 1)); // remove args
                op->extractSelf();
                return;
            };
            case OP_DEBUG_PRINT:
                m_convertDebugPrint(op);
                return;
            default:
                misc::error(TAG) << "Cannot convert op " << ACCENT << op->name() << RST;
                abort();
        }
    }

    void run(Manager *mgr) const override
    {
        if (!mgr->backend() || mgr->backend()->archName() != "ivm") {
            misc::info(TAG) << "Backend is not set to `ivm`, skipping";
            return;
        }

        misc::info(TAG) << "Work is being done";
        for (auto func : mgr->module()->funcImpls()) {
            std::unordered_map<ArgDecl*, int> offsets;
            // Stack frame:
            // <0 - locals
            // 0 - prev stack frame
            // 8 - return address
            // >=16 - args
            int off = 16; 
            for (auto arg : func->decl()->args()) {
                assert(arg->type()->size() <= 8); // dumb calling convention: all args are 8 bytes, on datastack
                offsets[arg] = off;
                off += 8;
            }
            for (auto block : func->blocks()) {
                for (auto it = block->operations().last(); it != nullptr;) {
                    auto n = it->prev();
                    m_convert(it, offsets);
                    it = n;
                }
            }
            auto entry = func->blocks().first();
            auto ins = entry->operations().inserterBefore(entry->operations().first());
            ins.create(IVM_R_SPUSH, nullptr);
            ins.create(IVM_R_SFBEGIN, nullptr);
        }
        misc::verb(TAG) << "Resulting IR:\n" << misc::beginBlock << *mgr->module() << misc::endBlock;

    }
};

extern "C" Mod *modlib_create(ModManager *mm)
{
    return new HL2LL();
}
