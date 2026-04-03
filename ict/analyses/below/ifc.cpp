#include "ict/analyses/below.hpp"
#include "misclib/dump_stream.hpp"
using namespace misc::color;

namespace ict::an {

void BelowAnalysis::dump(std::ostream &os) const
{
    os << BOLD << PURPLE << "= BelowAnalysis\n" << RST << misc::beginBlock;
    for (auto &func : m_mgr->module()->children()) {
        os << BOLD << "@" << func->name() << NORMAL << DGRAY << ":\n" << misc::beginBlock;
        for (auto &block : func->children()) {
            os << GREEN << "%" << block->slotInParent() << RST;
            if (!block->name().empty())
                os << " (named " << GREEN << "%" << block->name() << RST << ")";
            os << " below" << GREEN;
            for (auto i : m_below[func->slotInParent()][block->slotInParent()])
                os << " %" << i->slotInParent();
            if (m_below[func->slotInParent()][block->slotInParent()].empty())
                os << RST << " none";
            os << RST << ", above" << GREEN;
            for (auto i : m_above[func->slotInParent()][block->slotInParent()])
                os << " %" << i->slotInParent();
            if (m_above[func->slotInParent()][block->slotInParent()].empty())
                os << RST << " none";
            os << RST << "\n";
        }
        os << RST << misc::endBlock;
    }
    os << misc::endBlock;
}

};
