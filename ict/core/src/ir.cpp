#include "ict/ir.hpp"
#include "ict/ir-macro.hpp"
#include "ict/mod.hpp"
#include "misclib/dump_stream.hpp"
#include <iomanip>
#include <ostream>
#include <sstream>

using namespace misc::color;

#define VREG_COLOR BLUE
#define NUM_COLOR  YELLOW
#define OP_COLOR   CYAN
#define BLOCK_COLOR GREEN
#define FUNC_COLOR BOLD
#define KW_COLOR PURPLE

namespace ict {

void VRegArg::dump(std::ostream &os) const {
    const Op *op = ptr();
    const Block *blk = op->parent();
    if (!op->vregName().empty())
        os << VREG_COLOR << "%" << op->vregName() << RST;
    else
        os << VREG_COLOR << "%" << blk->slotInParent() << "." << op->slotInParent() << RST;
}

void ConstArg::dump(std::ostream &os) const {
    os << NUM_COLOR << m_value << RST;
}

void BlockArg::dump(std::ostream &os) const {
    const Block *blk = ptr();
    if (blk->name().empty())
        os << BLOCK_COLOR << "%" << blk->slotInParent() << RST;
    else
        os << BLOCK_COLOR << "%" << blk->name() << RST;
}
View OpKind::name(Manager *mgr) const {
    if (m_value < 0) {
        return (mgr && mgr->backend()) ? mgr->backend()->lowOp2str(m_value) : "(low-level)";
    }
    switch(m_value) {
#define X(id, term, vreg, name, textname) case name: return textname;
ICT_FOR_ALL_OPS(X)
#undef X
        default:
            return "(bad op kind!)";
    }
}

bool OpKind::doesReturn(Manager *mgr) const {
    if (m_value < 0)
        return mgr && mgr->backend() && mgr->backend()->lowOpReturns(m_value);
    switch(m_value) {
#define X(id, term, vreg, name, textname) case name: return vreg;
ICT_FOR_ALL_OPS(X)
#undef X
        default:
            return false;
    }
}
    
OpKind OpKind::parse(View iname) {
#define X(id, term, vreg, name, textname) if (iname == textname) return name;
ICT_FOR_ALL_OPS(X)
#undef X
    return UNKNOWN_OP;
}

void Op::dump(std::ostream &os) const {
    const Block *blk = parent();
    auto mgr = parent()->parent()->parent()->manager();
    if (kind().doesReturn(mgr)) {
        if (vregName().empty()) {
            std::ostringstream oss;
            oss << blk->slotInParent() << "." << slotInParent();
            os << VREG_COLOR << "%" << std::setw(5) << std::left << oss.str() << RST << " ";
        } else {
            os << VREG_COLOR << "%" << std::setw(5) << std::left << vregName() << RST << " ";
        }
    } else {
        os << "       ";
    }
    os << OP_COLOR << std::left << std::setw(16) << kind().name(mgr) << RST;
    for (const auto &arg : children())
        os << " " << *arg;
    os << DGRAY << ";\n" << RST;
}

void Op::replaceRefsWith(Op *op) {
    for (size_t i = refs().size() - 1; i != (size_t) -1; --i)
        refs()[i]->replaceWith(VRegArg::create(op));
}

UPtr<Op> Op::extractSelf() {
    return parent()->extract(slotInParent());
}

void Block::dump(std::ostream &os) const {
    if (name().empty())
        os << BLOCK_COLOR << "%" << slotInParent() << ":" << RST << "\n";
    else
        os << BLOCK_COLOR << "%" << name() << ":" << RST << "\n";
    os << misc::beginBlock;
    for (const auto &child : children())
        os << *child;
    os << misc::endBlock;
}

void Function::dump(std::ostream &os) const {
    os << KW_COLOR << "func_impl " << RST << FUNC_COLOR << "@" << name() << DGRAY << "\n{\n" << RST;
    for (const auto &i : children())
        os << *i;
    os << DGRAY << "}\n" << RST;
}

void Module::dump(std::ostream &os) const {
    for (const auto &i : children())
        os << *i;
}

};
