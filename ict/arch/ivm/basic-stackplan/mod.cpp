/**
 * \file
 * \brief IVM Stack slot planner -- basic version
 *
 * This just stores each thing in its own slot, without slot reuse.
 */

#include "ict/mod.hpp"
#include "ict/arch/ivm.hpp"
#include "ict/ir.hpp"
#include "misclib/dump_stream.hpp"
#include <functional>
using namespace ict;
using namespace ict::ar::ivm;
using namespace misc::color;

#define TAG "ict.ivm.bs-stackplan"

class StackPlanner : public Pass
{
    int order() const override { return 100; };
    std::string_view id() const override { return "ict.ivm.basic-stackplan"; }
    std::string_view brief() const override { return "ICT IVM pass allocate stack slots -- version which gives a new slot for each variable"; }

    void m_forEachOpOfType(Function *func, OpKind kind, const std::function<void(Op*)> &fn) const
    {
        for (auto &block : func->children())
            for (auto &op : block->children())
                if (op->kind() == kind)
                    fn(op.get());
    }

    void m_function(Function *func) const
    {
        misc::verb(TAG) << "Layouting function " << BOLD << "@" << func->name() << RST;

        std::unordered_map<Op*, Integer> off;
        Integer curPos = 0;
        
        m_forEachOpOfType(func, IVM_FROMSTACK, [&off, &curPos](Op *op){
            curPos -= 8;
            off[op] = curPos;
            misc::verb(TAG) << "    Var " << ACCENT << "%" << op->parent()->slotInParent()
                << "." << op->slotInParent() << RST << " @ " << op << " -> slot " << ACCENT << curPos << RST;
        });

        m_forEachOpOfType(func, IVM_TOSTACK, [&off](Op *op){
            auto to = op->varg(0)->ptr();
            assert(off.count(to) != 0);
            op->replaceWith(Op::create(IVM_R_SFLOAD64, off.at(to)));
        });

        m_forEachOpOfType(func, IVM_FROMSTACK, [&off, &curPos](Op *op){
            op->replaceWith(Op::create(IVM_R_SFSTORE64, off.at(op)));
        });

        misc::verb(TAG) << "    Using " << ACCENT << -curPos << RST << " bytes for stack slots";

        auto block = func->child(0).ptr();
        size_t pos = 0;
        while (pos < block->size() && block->child(pos)->kind() != IVM_R_SFBEGIN)
            ++pos;
        if (pos == block->size()) {
            misc::error(TAG) << "Cannot find stack frame creation in " << ACCENT << "@" << func->name() << RST;
            abort();
        }
        block->insert(pos+1, Op::create(IVM_R_SADD, curPos));
    }

    void run(Manager *mgr) const override
    {
        if (mgr->backend()->archName() != "ivm") {
            misc::info(TAG) << "Backend is not set to `ivm`, skipping";
            return;
        }

        misc::info(TAG) << "Work is being done";
        for (auto &func : mgr->module()->children())
            m_function(func.get());

        misc::verb(TAG) << "Resulting IR:\n" << misc::beginBlock << *mgr->module() << misc::endBlock;
    }
};

extern "C" Mod *modlib_create(ModManager *mm)
{
    return new StackPlanner();
}
