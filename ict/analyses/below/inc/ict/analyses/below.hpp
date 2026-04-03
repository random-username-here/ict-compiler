/**
 * \file
 * \brief If one block will be visited on some path to other block
 */
#pragma once
#include "ict/mod.hpp"
#include <vector>
#include <unordered_set>

namespace ict::an {
class BelowAnalysis : 
        public Analysis
{
#ifdef ICT_AN_IMPL
    friend class ICT_AN_IMPL;
#endif
    // [func][block] -> list of blocks this is below 
    std::vector<std::vector<std::unordered_set<Block*>>> m_below;
    std::vector<std::vector<std::unordered_set<Block*>>> m_above;
    Manager *m_mgr;
public:

    bool strictlyBelow(Block *a, Block *b) const
    {
        assert(a->parent() == b->parent());
        return m_below[a->parent()->slotInParent()][a->slotInParent()].count(b);
    }

    bool strictlyAbove(Block *a, Block *b) const { return strictlyBelow(b, a); }
    bool below(Block *a, Block *b) const { return a == b || strictlyBelow(a, b); }
    bool above(Block *a, Block *b) const { return a == b || strictlyAbove(a, b); }

    bool strictlyBelow(Op *a, Op *b) const
    {
        if (a->parent() != b->parent())
            return below(a->parent(), b->parent());
        else
            return a->slotInParent() > b->slotInParent();
    }

    bool strictlyAbove(Op *a, Op *b) const { return strictlyBelow(b, a); }
    bool below(Op *a, Op *b) const { return a == b || strictlyBelow(a, b); }
    bool above(Op *a, Op *b) const { return a == b || strictlyAbove(a, b); }

    const std::unordered_set<Block*> &allStrictlyBelow(Block *b) const
    {
        return m_below[b->parent()->slotInParent()][b->slotInParent()];
    }
    const std::unordered_set<Block*> &allStrictlyAbove(Block *b) const
    {
        return m_above[b->parent()->slotInParent()][b->slotInParent()];
    }

    void dump(std::ostream &os) const;
};

};
