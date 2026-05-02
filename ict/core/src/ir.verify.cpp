// TODO: comparsion validators
// TODO: br/bc validators
// TODO: alloca/load/store validators
// TODO: call/ret/arg validators
// TODO: dominance
// TODO: ... and more checks

#include "ict/ir.hpp"
#include "ict/mod.hpp"
#include "misclib/dump_stream.hpp"
namespace ict {

#define TAG "ict.verify"
using namespace misc::color;

struct funcId { const FunctionImpl *fi; funcId(const FunctionImpl *fi) :fi(fi) {} };
struct blockId { const BasicBlock *bb; blockId(const BasicBlock *bi) :bb(bi) {} };
struct opId { const Operation *op; opId(const Operation *op) :op(op) {} };

std::ostream &operator<<(std::ostream &os, funcId lf) {
    os << BOLD << "@" << lf.fi->decl()->name() << RST;
    return os;
}

std::ostream &operator<<(std::ostream &os, blockId lb) {
    os << funcId(lb.bb->parent());
    os << DGRAY << ":" << GREEN << "%";
    if (!lb.bb->name().empty()) os << lb.bb->name();
    else os << lb.bb;
    return os;
}

std::ostream &operator<<(std::ostream &os, opId lb) {
    os << blockId(lb.op->parent());
    os << DGRAY << ":";
    size_t pos = 0;
    for (const Operation *o = lb.op; o != nullptr; o = o->prev())
        ++pos;
    os << YELLOW << pos << DGRAY << "(" << BLUE << lb.op->name() << DGRAY << ")" << RST;
    return os;
}

struct foo {
    virtual void dump(std::ostream &os) const {}
};

bool checkMathOps(const Operation *op) {
    if (op->args().size() != 2) {
        misc::error(TAG) << opId(op) << " requires exactly 2 arguments";
        return false;
    } 

    const Type *computedRetType = nullptr;

    bool ok = true;
    for (size_t i = 0; i < 2; ++i) {
        if (!op->arg_v(i)) {
            misc::error(TAG) << opId(op) << "'s " << (i+1) << " argument must be vreg";
            ok = false;
            continue;
        }
        auto from = op->arg_v(i)->ptr();
        if (from->returnType()->isVoid()) {
            misc::error(TAG) << opId(op) << "'s " << (i+1) << " argument is void";
            ok = false;
        } else if (!from->returnType()->isSimple()) {
            misc::error(TAG) << opId(op) << "'s " << (i+1) << " argument is not of a simple type";
            ok = false;
        } else if (computedRetType && !computedRetType->isSameAs(from->returnType().get())) {
            if (!op->tparam()) {
                misc::error(TAG) << opId(op) << "'s arguments type mismatch, and no tparam is given: first is "
                    << *computedRetType << ", second is " << *from->returnType();
                ok = false;
            }
        } else {
            computedRetType = from->returnType().get();
        }
    }

    return ok;
}

using OpChecker = bool (*)(const Operation *op);
static const std::unordered_map<int, OpChecker> l_checkers = {
    { OP_ADD, checkMathOps },
    { OP_SUB, checkMathOps },
    { OP_DIV, checkMathOps },
    { OP_MUL, checkMathOps },
    { OP_MOD, checkMathOps },
};

bool Operation::verifyArgsOnly() const {
    if (!l_checkers.count(kind())) {
        misc::warn(TAG) << opId(this) << " has unknown kind or is missing a validator";
        return true;
    }
    return l_checkers.at(kind())(this);
}

bool checkOp(const Operation *op) {
    if (op->requiresExplicitType() && !op->tparam()) {
        misc::error(TAG) << "Operation " << opId(op) << " requires an explicit tparam";
        return false;
    }
    if (op->isLowLevel()) {
        auto mgr = Manager::main();
        if (!mgr) {
            misc::error(TAG) << "Manager is not created, but " << opId(op) << " is low-level op";
            return false;
        } else if (!mgr->backend()) {
            misc::error(TAG) << "Manager is missing backend, but " << opId(op) << " is low-level op";
            return false;
        } else {
            return true; // backend will verify its thing separately
        }
    }
    if (!op->returnType()) {
        misc::error(TAG) << "Operation " << opId(op) << " is missing return type";
        return false;
    }
    // TODO: check strict domination
    return op->verifyArgsOnly();
}

bool checkBlock(const BasicBlock *blk) {
    bool ok = true;
    if (blk->operations().size() == 0) {
        misc::error(TAG) << "Block " << blockId(blk) << " has no operations";
        return false;
    }
    if (!blk->operations().last()->isTerminal()) {
        misc::error(TAG) << "Last block's operation " << opId(blk->operations().last()) << " is not terminal";
        ok = false;
    }

    for (auto op : blk->operations()) {
        if (op != blk->operations().last() && op->isTerminal()) {
            misc::error(TAG) << "Terminal operation " << opId(op) << " not last in the block";
            ok = false;
        }
        ok &= checkOp(op);
    }

    return ok;
}

bool checkFunction(const FunctionImpl *fi) {
    bool ok = true;
    if (fi->blocks().size() == 0) {
        misc::error(TAG) << "Function " << funcId(fi) << " has no blocks";
        return false;
    }
    for (auto i : fi->blocks())
        ok &= checkBlock(i);
    return ok;
}

bool Module::verify() const {
    if (!Manager::main())
        misc::warn(TAG) << "Manager not created, low-level ops are not supported";
    else if (!Manager::main()->backend())
        misc::warn(TAG) << "Manager has no backend, low-level ops are not supported";

    bool ok = true;
    for (auto i : impls()) {
        if (auto func = dynamic_cast<const FunctionImpl*>(i))
            ok &= checkFunction(func);
    }
    return ok;
}

};
