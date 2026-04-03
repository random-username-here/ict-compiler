#define ICT_AN_IMPL CrossBlockAnalyzer
#include "misclib/dump_stream.hpp"
#include "ict/analyses/crossblock.hpp"
#include "ict/analyses/below.hpp"
#include "ict/mod.hpp"

#define TAG "ict.an.crossblock"

namespace ict::an {

class CrossBlockAnalyzer :
        public Analyzer 
{
    size_t analysisId() const override { return typeid(CrossBlockAnalysis).hash_code(); }
    std::string_view id() const override { return "ict.an.crossblock"; }
    std::string_view brief() const override { return "ICT analysis to determine which blocks vreg passes through"; }
    
    void m_runForFunction(const BelowAnalysis &ba, CrossBlockAnalysis &an, Function &fn) const
    {
        // o ∈ incoming(B) : def(o) > B and B >= some use(o)
        // o ∈ outgoing(B) : def(o) >= B and B > some use(o)

        size_t fi = fn.slotInParent();
        an.m_binfo[fi].resize(fn.size());

        for (auto &through : fn.children()) {
            size_t bi = through->slotInParent();
            for (auto &from : fn.children()) {
                bool definedAbove = false;
                if (ba.strictlyAbove(from.get(), through.get())) // def(o) > B
                    definedAbove = true;
                else if (from.get() != through.get()) // def(o) != B
                    continue;

                for (auto &op : from->children()) {
                    bool usedBelow = false, usedHere = false;
                    for (auto &use : op->refs()) {
                        auto useBlock = use->parent()->parent();
                        if (ba.strictlyBelow(useBlock, through.get()))
                            usedBelow = true;
                        else if (useBlock == through.get())
                            usedHere = true;
                    }
                    if (definedAbove && (usedBelow || usedHere))
                        an.m_binfo[fi][bi].incoming.insert(op.get());
                    if (usedBelow) // and defined here/above
                        an.m_binfo[fi][bi].outgoing.insert(op.get());
                    if (!definedAbove && usedBelow)
                        an.m_binfo[fi][bi].created.insert(op.get());
                }
            }
        }
    }

    UPtr<Analysis> run(Manager *mgr) const override
    {
        misc::info(TAG) << "Running crossblock analysis";
        auto o_an = std::make_unique<CrossBlockAnalysis>();
        o_an->m_binfo.resize(mgr->module()->size());
        o_an->m_mgr = mgr;
        auto ba = mgr->analysis<BelowAnalysis>();
        for (auto &func : mgr->module()->children())
            m_runForFunction(*ba, *o_an, *func);
        return o_an;
    }
};

extern "C" Mod *modlib_create(ModManager *mm)
{
    return new ict::an::CrossBlockAnalyzer();
}
};
