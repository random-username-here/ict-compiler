/**
 * \file
 * \brief Finding vregs which are created in one block, and then cross block boundary.
 */
#pragma once
#include "ict/mod.hpp"
#include <unordered_set>

namespace ict::an {

class CrossBlockAnalysis :
        public Analysis
{
#ifdef ICT_AN_IMPL
    friend class ICT_AN_IMPL;
#endif
    struct BlockInfo {
        std::unordered_set<Op*> incoming, outgoing, created;
    };
    std::vector<std::vector<BlockInfo>> m_binfo;
    Manager *m_mgr;
public:

    /// Get all vregs which were created before, and will be used by block and following blocks
    const std::unordered_set<Op*> &incoming(Block *block) const 
        { return m_binfo[block->parent()->slotInParent()][block->slotInParent()].incoming; }
    
    /// Get all vregs created in that block, which will be used after the block
    const std::unordered_set<Op*> &created(Block *block) const
        { return m_binfo[block->parent()->slotInParent()][block->slotInParent()].created; }

    /// Get all vregs which will be used in following blocks
    /// Includes vregs created in this block.
    const std::unordered_set<Op*> outgoing(Block *block) const
        { return m_binfo[block->parent()->slotInParent()][block->slotInParent()].outgoing; }


    /// If op is only used in its block
    bool isLocal(Op *op) const
    {
        size_t bid = op->parent()->slotInParent();
        size_t fid = op->parent()->parent()->slotInParent();
        return m_binfo[fid][bid].created.count(op) == 0;
    }

    void dump(std::ostream &os) const;
};

};
