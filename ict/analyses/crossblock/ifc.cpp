#include "ict/analyses/crossblock.hpp"
#include "misclib/dump_stream.hpp"
using namespace misc::color;

namespace ict::an {

static void l_opNameDump(std::ostream &os, Op *op)
{
    if (op->vregName().empty())
        os << "%" << op->slotInParent() << " ";
    else
        os << "%" << op->vregName() << " ";
}

void CrossBlockAnalysis::dump(std::ostream &out) const
{
    out << BOLD << PURPLE << "= CrossBlockAnalysis\n" << RST << misc::beginBlock;
    for (auto &func : m_mgr->module()->children()) {
        out << BOLD << "@" << func->name() << NORMAL << DGRAY << ":\n" << RST << misc::beginBlock;
        for (auto &block : func->children()) {
            out << GREEN << "%";
            if (block->name().empty())
                out << block->slotInParent();
            else
                out << block->name();
            out << misc::beginBlock;
            out << RST << "\nincoming : " << BLUE;
            for (auto i : incoming(block.get())) l_opNameDump(out, i);
            out << RST << "\ncreated  : " << BLUE;
            for (auto i : created(block.get())) l_opNameDump(out, i);
            out << RST << "\noutgoing : " << BLUE;
            for (auto i : outgoing(block.get())) l_opNameDump(out, i);
            out << "\n" << misc::endBlock;
        }
        out << misc::endBlock;
    }
    out << misc::endBlock;
}

};
