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

    void m_forEachOpOfType(FunctionImpl *func, int kind, const std::function<void(Operation*)> &fn) const
    {
        for (auto block : func->blocks()) {
            for (auto op = block->operations().first(); op != nullptr;) {
                auto next = op->next();
                if (op->kind() == kind)
                    fn(op);
                op = next;
            }
        }
    }

    void m_function(FunctionImpl *func) const
    {
        misc::verb(TAG) << "Layouting function " << BOLD << "@" << func->decl()->name() << RST;

        std::unordered_map<Operation*, Integer> off;
        Integer curPos = 0;
        
        m_forEachOpOfType(func, IVM_FROMSTACK, [&off, &curPos](Operation *op){
            curPos -= 8;
            off[op] = curPos;
            misc::verb(TAG) << "    Var " << ACCENT << "%" << op << RST << " @ " << op << " -> slot " << ACCENT << curPos << RST;
        });

        m_forEachOpOfType(func, IVM_TOSTACK, [&off](Operation *op){
            auto to = op->arg_v(0)->ptr();
            assert(off.count(to) != 0);
            op->parentList()->createBefore(op, IVM_R_SFLOAD64, nullptr, off.at(to));
            op->extractSelf();
        });

        m_forEachOpOfType(func, IVM_FROMSTACK, [&off, &curPos](Operation *op){
            op->parentList()->createBefore(op, IVM_R_SFSTORE64, nullptr, off.at(op));
            op->extractSelf();
        });

        misc::verb(TAG) << "    Using " << ACCENT << -curPos << RST << " bytes for stack slots";

        auto block = func->blocks().first();
        Operation *sfbegin = nullptr;
        for (auto op = block->operations().first(); op != nullptr && sfbegin == nullptr; op = op->next()) {
            if (op->kind() == IVM_R_SFBEGIN)
                sfbegin = op;
        }
        if (!sfbegin) {
            misc::error(TAG) << "Cannot find stack frame creation in " << ACCENT << "@" << func->decl()->name() << RST;
            abort();
        }
        block->operations().createAfter(sfbegin, IVM_R_SADD, nullptr, curPos);
    }

    void run(Manager *mgr) const override
    {
        if (mgr->backend()->archName() != "ivm") {
            misc::info(TAG) << "Backend is not set to `ivm`, skipping";
            return;
        }

        misc::info(TAG) << "Work is being done";
        for (auto func : mgr->module()->funcImpls())
            m_function(func);

        misc::verb(TAG) << "Resulting IR:\n" << misc::beginBlock << *mgr->module() << misc::endBlock;
    }
};

extern "C" Mod *modlib_create(ModManager *mm)
{
    return new StackPlanner();
}
