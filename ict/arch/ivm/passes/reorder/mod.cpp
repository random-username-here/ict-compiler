#include "ict/mod.hpp"
#include "ict/ir.hpp"
#include "ict/arch/ivm.hpp"
#include "misclib/dump_stream.hpp"

#define TAG "ict.ivm.reorder"

namespace ivm { using namespace ict::ar::ivm; }

struct Stats {
    size_t toStackCount = 0, fromStackCount = 0, pairsFound = 0, delCount = 0;
};

class Reorderer : public ict::Pass {
    int order() const override { return 80; };
    std::string_view id() const override { return "ict.ivm.reorder"; }
    std::string_view brief() const override { return "ICT IVM pass to reorder operations so we don't spill vreg which is used only once"; }

    bool isNotSafeToReorder(ict::Operation *operation) const {
        return operation->hasSideEffects() || operation->hasTag("noreorder");
    }

    bool isLoad(ict::Operation *op) const {
        switch (op->kind()) {
            case ivm::IVM_R_SFLOAD8: case ivm::IVM_R_SFLOAD16: case ivm::IVM_R_SFLOAD32: case ivm::IVM_R_SFLOAD64:
            case ivm::IVM_R_GET8: case ivm::IVM_R_GET16: case ivm::IVM_R_GET32: case ivm::IVM_R_GET64:
                return true;
            default:
                return false;
        }
    }

    bool canExchange(ict::Operation *a, ict::Operation *b) const {
        return isLoad(a) && isLoad(b);
    }

    void reorderBlock(ict::BasicBlock *blk, Stats &stats) const {
        for (auto op : blk->operations()) {
            if (op->kind() == ivm::IVM_TOSTACK) ++stats.toStackCount;
            if (op->kind() == ivm::IVM_FROMSTACK) {
                ++stats.fromStackCount;
                if (op->refs().size() == 1)
                    ++stats.pairsFound;
            }
        }

        for (ict::Operation *toStack = blk->operations().last(); toStack != nullptr; toStack = toStack->prev()) {
            // we want to pull FromStack to ToStack, and collapse them

            if (toStack->kind() != ivm::IVM_TOSTACK)
                continue;
            auto fromStack = toStack->arg_v(0)->ptr();
            if (fromStack->kind() != ivm::IVM_FROMSTACK)
                continue;
            if (fromStack->parent() != toStack->parent())
                continue; // TODO: pull them if source dominates op and op is not in a cycle lower than source
            if (fromStack->refs().size() != 1)
                continue;

            auto operation = fromStack->arg_v(0)->ptr();

            if (isNotSafeToReorder(operation)) {
                bool blocking = false;
                for (auto i = operation->next(); i != toStack; i = i->next()) {
                    if (isNotSafeToReorder(i) && !canExchange(operation, i)) {
                        misc::verb(TAG) << "Cannot move: " << *operation << "Because blocked by: " << *i;
                        blocking = true;
                    }
                }
                if (blocking)
                    continue;
            }
            // now we are going to pull that operation's ToStack and operation
            blk->operations().insertBefore(toStack, operation->extractSelf());
            for (auto i : operation->args()) {
                if (auto vreg = dynamic_cast<ict::VRegArg*>(i)) {
                    if (vreg->ptr()->kind() != ivm::IVM_TOSTACK)
                        continue;
                    blk->operations().insertBefore(operation, vreg->ptr()->extractSelf());
                }
            }
            // anihillate ToStack/FromStack
            ++stats.delCount;
            toStack->replaceRefsWith(operation);
            fromStack->extractSelf();
            toStack->extractSelf();
            toStack = operation;

        }
    }

    void run(ict::Manager *mgr) const override {
        Stats stats;
        misc::info(TAG) << "Reorder pass doing its thing...";
        for (auto impl : mgr->module()->impls())
            if (auto func = dynamic_cast<ict::FunctionImpl*>(impl))
                for (auto block : func->blocks())
                    reorderBlock(block, stats);
        misc::info(TAG) << "Found " << stats.fromStackCount << " FromStack, " 
                << stats.toStackCount << " ToStack, " << stats.pairsFound << " single def-use pairs,\ncollapsed " 
                << stats.delCount << " of them ==> " << 100 * stats.delCount / stats.pairsFound << "% decrease in pairs, "
                << 100 * stats.delCount / stats.toStackCount << "% decrease in slots";
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new Reorderer();
}
