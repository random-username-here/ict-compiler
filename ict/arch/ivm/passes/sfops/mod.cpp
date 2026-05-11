#include "ict/mod.hpp"
#include "ict/ir.hpp"
#include "ict/arch/ivm.hpp"
#include "misclib/dump_stream.hpp"

#define TAG "ict.ivm.sfops"
namespace ivm { using namespace ict::ar::ivm; }

struct Stats {
    size_t loads = 0, stores = 0, changedLoads = 0, changedStores = 0;
};

class SfloadReplacer : public ict::Pass {
    int order() const override { return 85; };
    std::string_view id() const override { return "ict.ivm.sfops"; }
    std::string_view brief() const override { return "ICT IVM pass to convert reads/writes to variables in stackframe to sfload/sfstore commands"; }
    // TODO: pattern matching

    bool isMemOp(int kind) const {
        switch (kind) {
            case ivm::IVM_R_GET8: case ivm::IVM_R_GET16: case ivm::IVM_R_GET32: case ivm::IVM_R_GET64:
            case ivm::IVM_R_PUT8: case ivm::IVM_R_PUT16: case ivm::IVM_R_PUT32: case ivm::IVM_R_PUT64:
                return true;
            default:
                return false;
        }
    }

    int cvtMemOp(int kind) {
        switch (kind) {
            case ivm::IVM_R_GET8: return ivm::IVM_R_SFLOAD8;
            case ivm::IVM_R_GET16: case ivm::IVM_R_GET32: case ivm::IVM_R_GET64:
            case ivm::IVM_R_PUT8: case ivm::IVM_R_PUT16: case ivm::IVM_R_PUT32: case ivm::IVM_R_PUT64:

        }
    }

    void run(ict::Manager *mgr) const override {
        Stats stats;
        misc::info(TAG) << "Reorder pass doing its thing...";
        for (auto impl : mgr->module()->impls()) {
            if (auto func = dynamic_cast<ict::FunctionImpl*>(impl)) {
                for (auto block : func->blocks()) {
                    for (auto op : block->operations()) {
                        if (!isMemOp(op->kind())) continue;
                        auto add = op->arg_v(0)->ptr();
                        if (add->kind() != ivm::IVM_R_ADD && add->kind() != ivm::IVM_R_SUB) continue;
                        auto gsf = add->arg_v(0)->ptr();
                        if (gsf->kind() != ivm::IVM_R_GSF) continue;
                        auto off = add->arg_v(1)->ptr();
                        if (off->kind() != ivm::IVM_R_PUSH) continue;
                        auto sf = 
                    }
                }
            }
        }
                    
        misc::info(TAG) << "Found " << stats.fromStackCount << " FromStack, " 
                << stats.toStackCount << " ToStack, " << stats.pairsFound << " single def-use pairs,\ncollapsed " 
                << stats.delCount << " of them ==> " << 100 * stats.delCount / stats.pairsFound << "% decrease in pairs, "
                << 100 * stats.delCount / stats.toStackCount << "% decrease in slots";
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new Reorderer();
}
