#define ICT_AN_IMPL BelowAnalyzer
#include "ict/mod.hpp"
#include "ict/analyses/below.hpp"
#include "ict/ir.hpp"
#include "misclib/dump_stream.hpp"
using namespace misc::color;
namespace ict::an {

class BelowAnalyzer :
        public Analyzer
{

    size_t analysisId() const override { return typeid(BelowAnalysis).hash_code(); }
    std::string_view id() const override { return "ict.an.below"; }
    std::string_view brief() const override { return "ICT analysis to determine which blocks can be reached from given one"; }
    
    void m_walk(BelowAnalysis &res, Block *src, Block *blk, std::vector<bool> &visited) const
    {
        if (visited[blk->slotInParent()])
            return;
        visited[blk->slotInParent()] = true;

        size_t func_idx = src->parent()->slotInParent();
        if (src != blk) {
            res.m_above[func_idx][src->slotInParent()].insert(blk);
            res.m_below[func_idx][blk->slotInParent()].insert(src);
        }

        auto lastNode = blk->child(blk->size()-1).ptr();
        if (lastNode->kind() != BC && lastNode->kind() != BR)
            return;

        for (auto &arg : lastNode->children()) {
            auto barg = dynamic_cast<BlockArg*>(arg.get());
            if (!barg) continue;
            m_walk(res, src, barg->ptr(), visited);
        }
    }

    UPtr<Analysis> run(Manager *mgr) const override
    {
        misc::info("ict.an.below") << "Running below analysis";

        auto res = std::make_unique<BelowAnalysis>();
        res->m_below.resize(mgr->module()->size());
        res->m_above.resize(mgr->module()->size());
        res->m_mgr = mgr;

        for (auto &func : mgr->module()->children()) {
            res->m_below[func->slotInParent()].resize(func->size());
            res->m_above[func->slotInParent()].resize(func->size());
            for (auto &block : func->children()) {
                std::vector<bool> used(func->size(), false);
                m_walk(*res, block.get(), block.get(), used);
            }
        }

        return res;
    }
};
};

extern "C" Mod *modlib_create(ModManager *mm)
{
    return new ict::an::BelowAnalyzer();
}
