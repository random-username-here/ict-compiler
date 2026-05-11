#include "ict/mod.hpp"
#include "ict/ir.hpp"

class Dehumanifier : public ict::Pass {
    int order() const override { return -1000; };
    std::string_view id() const override { return "ict.dehumanify"; }
    std::string_view brief() const override { return "ICT pass to pull constants out of non-`Const` operands"; }

    void run(ict::Manager *mgr) const override {
        for (auto impl : mgr->module()->impls()) {
            if (auto func = dynamic_cast<ict::FunctionImpl*>(impl)) {
                for (auto block : func->blocks()) {
                    for (auto op : block->operations()) {
                        if (op->kind() == ict::OP_CONST || op->kind() == ict::OP_GLOBALPTR || op->isLowLevel())
                            continue;
                        bool first = true;
                        for (auto arg : op->args()) {
                            if (auto constant = dynamic_cast<ict::ConstArg*>(arg)) {
                                auto cons = block->operations().createBefore(op, ict::OP_CONST, nullptr, constant->value());
                                constant->replace(ict::VRegArg::create(cons));
                            } else if (auto global = dynamic_cast<ict::GlobalArg*>(arg)) {
                                auto glb = block->operations().createBefore(op, ict::OP_GLOBALPTR, nullptr, global->ptr());
                                if (op->kind() == ict::OP_CALL && first) {
                                    auto func = dynamic_cast<ict::FunctionDecl*>(global->ptr());
                                    assert(func);
                                    op->tparam() = func->retType()->clone();
                                }
                                global->replace(ict::VRegArg::create(glb));
                            }
                            first = false;
                        }
                    }
                }
                func->invalidateAnalysis();
            }
        }
    }
};

extern "C" Mod *modlib_create(ModManager *mm) {
    return new Dehumanifier();
}
