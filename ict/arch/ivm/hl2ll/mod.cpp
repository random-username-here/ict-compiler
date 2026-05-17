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
    void m_replaceWithOneToOneMapping(Operation *op, int llirOpKind) const
    {
        auto inserter = op->parentList()->inserterBefore(op);
        bool inverse = m_hasInvertedOperandOrder(op->kind());
        std::vector<Operation*> ops;
        for (size_t i = (inverse ? op->args().size()-1 : 0);
             inverse ? (i != (size_t) -1) : (i < op->args().size());
             inverse ? --i : ++i
        ) {
            auto arg = op->arg(i);
            if (auto varg = dynamic_cast<VRegArg*>(arg)) {
                ops.push_back(inserter.create(IVM_TOSTACK, nullptr, varg->ptr()));
            } else {
                misc::error(TAG) << "Cannot comprehend arg " << *arg << " for one-to-one mapping";
                abort();
            }
        }
        auto inst = inserter.create(llirOpKind);
        for (auto i : ops)
            inst->args().createEnd<ict::VRegArg>(i);
        if (!op->returnType()->isVoid()) {
            auto load = inserter.create(IVM_FROMSTACK, op->returnType()->clone(), inst);
            op->replaceRefsWith(load);
        }
        op->extractSelf();
    }

    void m_convertDebugPrint(Operation *op) const
    {
        auto ins = op->parentList()->inserterBefore(op);
        for (auto arg : op->args()) {
            if (auto varg = dynamic_cast<VRegArg*>(arg)) {
                auto val = ins.create(IVM_TOSTACK, nullptr, varg->ptr());
                ins.create(IVM_DEBUG_PRINT_INT, nullptr, val);
            } else {
                misc::error(TAG) << "DebugPrint has unexpected arg " << *arg;
                abort();
            }
        }
        auto nl = ins.create(IVM_R_PUSH, nullptr, (Integer) '\n');
        ins.create(IVM_DEBUG_PRINT_CHAR, nullptr, nl);
        op->extractSelf();
    }

    void m_convert(Operation *op, std::unordered_map<ArgDecl*, int> argOffsets, FunctionImpl *inside) const
    {
        int o2o = m_getOneToOneMapping(op->kind());
        if (o2o) {
            m_replaceWithOneToOneMapping(op, o2o);
            return;
        }

        auto inserter = op->parentList()->inserterBefore(op);
        switch(op->kind()) {
            case OP_CONST: {
                auto v = inserter.create(IVM_R_PUSH, nullptr, op->arg_c(0)->value());
                auto val = inserter.create(IVM_FROMSTACK, nullptr, v);
                op->replaceRefsWith(val);
                op->extractSelf();
                return;
            }
            case OP_NEQ: {
                auto a = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr());
                auto b = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(1)->ptr());
                auto eq = inserter.create(IVM_R_EQ, nullptr, a, b);
                auto iszero = inserter.create(IVM_R_ISZERO, nullptr, eq);
                auto res = inserter.create(IVM_FROMSTACK, nullptr, iszero);
                op->replaceRefsWith(res);
                op->extractSelf();
                return;
            }
            case OP_RET: {
                if (op->args().size()) {
                    auto val = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr());
                }
                inserter.create(IVM_R_SFEND, nullptr)->addTag("noreorder");
                inserter.create(IVM_R_SADD, nullptr, inside->decl()->args().size() * 8)->addTag("noreorder"); // drop args
                inserter.create(IVM_R_SPOP, nullptr)->addTag("noreorder");
                inserter.create(IVM_R_RET, nullptr)->addTag("noreorder");

                op->extractSelf();
                return;
            }
            case OP_BR:
                inserter.create(IVM_R_RJMP, nullptr, op->arg_b(0)->ptr());
                op->extractSelf();
                return;
            case OP_BC: {
                auto val = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr());
                inserter.create(IVM_R_RJNZ, nullptr, op->arg_b(1)->ptr(), val);
                inserter.create(IVM_R_RJMP, nullptr, op->arg_b(2)->ptr());
                op->extractSelf();
                return;
            }
            case OP_ALLOCA: {
                size_t size = 0;
                ict::Operation *count = nullptr;
                if (op->args().size() == 1)
                    count = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr());
                else
                    count = inserter.create(IVM_R_PUSH, nullptr, -op->tparam()->size());
                inserter.create(IVM_R_SADD, nullptr, count);
                auto sp = inserter.create(IVM_R_GSP, nullptr);
                for (auto i : op->tags())
                    if (i.name.starts_with("var."))
                        sp->addTag(i); // keep var names
                auto res = inserter.create(IVM_FROMSTACK, nullptr, sp);
                op->replaceRefsWith(res);
                op->extractSelf();
                return;
            }
            case OP_STORE: {
                auto val = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(1)->ptr()); // val
                auto pos = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr()); // pos
                assert(op->tparam()->isSimple());
                switch (op->tparam()->size()) {
                    case 1: inserter.create(IVM_R_PUT8, nullptr, val, pos); break;
                    case 2: inserter.create(IVM_R_PUT16, nullptr, val, pos); break;
                    case 4: inserter.create(IVM_R_PUT32, nullptr, val, pos); break;
                    case 8: inserter.create(IVM_R_PUT64, nullptr, val, pos); break;
                    default: assert(false);
                }
                op->extractSelf();
                return;
            };

            case OP_LOAD: {
                auto pos = inserter.create(IVM_TOSTACK, nullptr, op->arg_v(0)->ptr()); // pos
                assert(op->returnType()->isSimple());
                ict::Operation *load = nullptr;
                switch (op->returnType()->size()) {
                    case 1: load = inserter.create(IVM_R_GET8, nullptr, pos); break;
                    case 2: load = inserter.create(IVM_R_GET16, nullptr, pos); break;
                    case 4: load = inserter.create(IVM_R_GET32, nullptr, pos); break;
                    case 8: load = inserter.create(IVM_R_GET64, nullptr, pos); break;
                    default: assert(false);
                }
                auto g = inserter.create(IVM_FROMSTACK, op->returnType()->clone(), load);
                op->replaceRefsWith(g);
                op->extractSelf();
                return;
            };

            case OP_ARGPTR: {
                auto sf = inserter.create(IVM_R_GSF, nullptr);
                auto off = inserter.create(IVM_R_PUSH, nullptr, argOffsets[op->arg_a(0)->ptr()]);
                auto ptr = inserter.create(IVM_R_ADD, nullptr, sf, off);
                auto g = inserter.create(IVM_FROMSTACK, Type::ptr_t(), ptr);
                op->replaceRefsWith(g);
                op->extractSelf();
                return;
            };
            case OP_CALL: {
                std::vector<ict::Operation*> refs;
                for (size_t i = op->args().size()-1; i != (size_t) -1; --i) {
                    refs.push_back(inserter.create(IVM_TOSTACK, nullptr, op->arg_v(i)->ptr()));
                }
                auto cl = inserter.create(IVM_R_CALL, nullptr);
                for (auto i : refs)
                    cl->args().createEnd<ict::VRegArg>(i);

                if (!op->returnType()->isVoid()) {
                    auto g = inserter.create(IVM_FROMSTACK, op->returnType()->clone(), cl);
                    op->replaceRefsWith(g);
                }
                op->extractSelf();
                return;
            };
            case OP_GLOBALPTR: {
                auto v = inserter.create(IVM_R_PUSH, nullptr, op->arg_g(0)->ptr());
                auto g = inserter.create(IVM_FROMSTACK, Type::ptr_t(), v);
                op->replaceRefsWith(g);
                op->extractSelf();
                return;
            }
            case OP_DEBUG_PRINT:
                m_convertDebugPrint(op);
                return;
            default:
                misc::error(TAG) << "Cannot convert op " << ACCENT << op->name() << RST;
                abort();
        }
    }

    bool run(Manager *mgr) const override
    {
        if (!mgr->backend() || mgr->backend()->archName() != "ivm") {
            misc::info(TAG) << "Backend is not set to `ivm`, skipping";
            return false;
        }

        misc::info(TAG) << "Work is being done";
        for (auto it : mgr->module()->impls()) {
            auto func = dynamic_cast<FunctionImpl*>(it);
            if (!func) continue;
            func->invalidateAnalysis();

            std::unordered_map<ArgDecl*, int> offsets;
            // Stack frame:
            // <0 - locals
            // 0 - prev stack frame
            // 8 - return address
            // >=16 - args
            int off = 8; 
            for (size_t i = func->decl()->args().size() - 1; i != (size_t) -1; --i) {
                auto arg = func->decl()->args()[i];
                assert(arg->type()->size() <= 8);
                offsets[arg] = off;
                off += 8;
            }
            for (auto block : func->blocks()) {
                for (auto it = block->operations().last(); it != nullptr;) {
                    auto n = it->prev();
                    m_convert(it, offsets, func);
                    it = n;
                }
            }
            auto entry = func->blocks().first();
            auto ins = entry->operations().inserterBefore(entry->operations().first());
            ins.create(IVM_R_SPUSH, nullptr)->addTag("noreorder");
            for (auto i : func->decl()->args())
                ins.create(IVM_R_SPUSH, nullptr)->addTag("noreorder")->addTag(Tag(std::string("arg.") + std::string(i->name())));
            ins.create(IVM_R_SFBEGIN, nullptr)->addTag("noreorder");
        }
        return true;
    }
};

extern "C" Mod *modlib_create(ModManager *mm)
{
    return new HL2LL();
}
