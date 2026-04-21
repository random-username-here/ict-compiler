#include "ict/ir.hpp"
#include "ict/ir-macro.hpp"
#include "ict/mod.hpp"
#include "misclib/dump_stream.hpp"
#include <cassert>
#include <ostream>

using namespace misc::color;

namespace ict {

FunctionDecl *Module::findFunc(View name) {
    for (auto i : funcDecls())
        if (i->name() == name)
            return i;
    return nullptr;
}
    
void Module::dump(std::ostream &os) const {
    os << PURPLE << BOLD << "Module " << RST << name() << "\n" << misc::beginBlock;
    os << DGRAY << "// decls:\n";
    for (auto i : funcDecls())
        i->dump(os);
    os << DGRAY << "// impls:\n";
    for (auto i : funcImpls())
        i->dump(os);
    os << misc::endBlock;
}

UPtr<ArgDecl> FunctionDecl::l_toArg(UPtr<Type> &&t) {
    return ArgDecl::create(std::move(t));
}

FunctionImpl *FunctionDecl::implement() {
    if (impl()) return nullptr;
    m_impl = parent()->funcImpls().createBefore(nullptr, this);
    return m_impl;
}

void FunctionDecl::dump(std::ostream &os) const {
    os << PURPLE << "func_decl " << RST << BOLD << "@" << name() << RST << DGRAY << " (" << RST;
    for (size_t i = 0; i < args().size(); ++i) {
        if (i != 0) os << DGRAY << ", " << RST;
        args()[i]->dump(os);
    }
    os << DGRAY << ") " << RST;
    retType()->dump(os);
    os << DGRAY << ";";
    if (!impl())
        os << " // decl-only";
    os << RST << "\n";
}

void ArgDecl::dump(std::ostream &os) const {
    if (!name().empty())
        os << RED << "%" << name() << " " << RST;
    os << *type();
}

BasicBlock *FunctionImpl::createBlock(View name) {
    return blocks().createBefore(nullptr, std::string(name));
}

void FunctionImpl::dump(std::ostream &os) const {
    os << PURPLE << "func_impl " << RST << BOLD << "@" << decl()->name() << RST
        << DGRAY << " {\n" << RST << misc::beginBlock;
    for (auto blk : blocks())
        blk->dump(os);
    os << misc::endBlock << DGRAY << "}\n" << RST;
}

std::vector<const BasicBlock*> BasicBlock::predecessors() const {
    std::vector<const BasicBlock*> blocks;
    for (auto i : refs()) {
        auto op = i->parent();
        if(!op->isTerminal()) continue;
        blocks.push_back(op->parent());
    }
    return blocks;
}

std::vector<const BasicBlock*> BasicBlock::successors() const {
    std::vector<const BasicBlock*> blocks;
    assert(operations().size() != 0);
    assert(operations().last()->isTerminal());
    for (auto i : operations().last()->args())
        if (auto ba = dynamic_cast<const BlockArg*>(i))
            blocks.push_back(ba->ptr());
    return blocks;
}

std::vector<BasicBlock*> BasicBlock::predecessors() {
    std::vector<BasicBlock*> blocks;
    for (auto i : refs()) {
        auto op = i->parent();
        if(!op->isTerminal()) continue;
        blocks.push_back(op->parent());
    }
    return blocks;
}

std::vector<BasicBlock*> BasicBlock::successors() {
    std::vector<BasicBlock*> blocks;
    assert(operations().size() != 0);
    assert(operations().last()->isTerminal());
    for (auto i : operations().last()->args())
        if (auto ba = dynamic_cast<BlockArg*>(i))
            blocks.push_back(ba->ptr());
    return blocks;
}

void BasicBlock::dump(std::ostream &os) const {
    os << GREEN << "%";
    if (!name().empty()) os << name();
    else os << this;
    os << DGRAY << ": // preds = ";
    auto preds = predecessors();
    if (preds.empty()) os << "(none)";
    for (auto i : preds) {
        os << "%";
        if (!i->name().empty()) os << i->name();
        else os << i;
        os << " ";
    }
    os << RST << "\n" << misc::beginBlock;
    for (auto i : operations())
        i->dump(os);
    os << misc::endBlock;
}

UPtr<Type> Operation::l_createRt() {
    if (isLowLevel())
        return Manager::main()->backend()->createReturnType(this);
    switch (kind()) {
        case OP_RET:
        case OP_STORE:
        case OP_BR:
        case OP_BC:
        case OP_DEBUG_PRINT:
            return Type::void_t();
        case OP_ALLOCA:
        case OP_ARGPTR:
            return Type::ptr_t();
        case OP_CALL:
            return arg_f(0)->ptr()->retType()->clone();
        case OP_CONST: 
        case OP_LOAD:
            return tparam().get() ? tparam()->clone() : Type::i64_t();
        case OP_ADD:
        case OP_SUB:
        case OP_DIV:
        case OP_MUL:
        case OP_MOD:
            if (tparam().get()) return tparam()->clone();
            return arg_v(0)->ptr()->returnType()->clone();
        case OP_LT: case OP_GT: case OP_GE:
        case OP_LE: case OP_EQ: case OP_NEQ:
            return Type::i8_t();
        default:
            misc::error("ict.ir") << "Unknown op type: " << kind();
            assert(false); // must specify
    }
}

UPtr<ConstArg> Operation::l_mapToArg(Integer i) { return ConstArg::create(i); }
UPtr<VRegArg>  Operation::l_mapToArg(Operation *o) { return VRegArg::create(o); }
UPtr<BlockArg> Operation::l_mapToArg(BasicBlock *b) { return BlockArg::create(b); }
UPtr<FuncArg>  Operation::l_mapToArg(FunctionDecl *f) { return FuncArg::create(f); }
UPtr<ArgArg>   Operation::l_mapToArg(ArgDecl *a) { return ArgArg::create(a); }

VRegArg     *Operation::arg_v(size_t i) { return dynamic_cast<VRegArg*>(arg(i)); }
ConstArg    *Operation::arg_c(size_t i) { return dynamic_cast<ConstArg*>(arg(i)); }
BlockArg    *Operation::arg_b(size_t i) { return dynamic_cast<BlockArg*>(arg(i)); }
FuncArg     *Operation::arg_f(size_t i) { return dynamic_cast<FuncArg*>(arg(i)); }
ArgArg      *Operation::arg_a(size_t i) { return dynamic_cast<ArgArg*>(arg(i)); }

const VRegArg     *Operation::arg_v(size_t i) const { return dynamic_cast<const VRegArg*>(arg(i)); }
const ConstArg    *Operation::arg_c(size_t i) const { return dynamic_cast<const ConstArg*>(arg(i)); }
const BlockArg    *Operation::arg_b(size_t i) const { return dynamic_cast<const BlockArg*>(arg(i)); }
const FuncArg     *Operation::arg_f(size_t i) const { return dynamic_cast<const FuncArg*>(arg(i)); }
const ArgArg      *Operation::arg_a(size_t i) const { return dynamic_cast<const ArgArg*>(arg(i)); }

bool Operation::isTerminal() const {
#define X(id, term, expl, name, textname) if (kind() == id) return term;
ICT_FOR_ALL_OPS(X)
#undef X
    return false;
}

bool Operation::requiresExplicitType() const {
    if (isLowLevel())
        return Manager::main()->backend()->lowOpRequiresType(kind());
#define X(id, term, expl, name, textname) if (kind() == id) return expl;
ICT_FOR_ALL_OPS(X)
#undef X
    return false;
}
View Operation::name() const {
    if (isLowLevel())
        return Manager::main()->backend()->lowOp2str(kind());
#define X(id, term, expl, name, textname) if (kind() == id) return textname;
ICT_FOR_ALL_OPS(X)
#undef X
    return "(unknown op type)";
}
    
int Operation::kindFromString(Manager *mgr, View name) {
 #define X(id, term, vreg, iname, textname) if (name == textname) return id;
ICT_FOR_ALL_OPS(X)
#undef X       
    return mgr ? mgr->backend()->str2lowOp(name) : 0;
}

void Operation::dump(std::ostream &os) const {
    if (!returnType()->isVoid()) {
        os << BLUE << "%";
        if (!vregName().empty()) os << vregName();
        else os << this;
        os << " " << RST;
    }
    os << name();
    if (tparam() || requiresExplicitType()) {
        os << DGRAY << "[" << RST;
        if (tparam().get()) {
            tparam()->dump(os);
        } else {
            os << RED << "tparam missing";
        }
        os << DGRAY << "]" << RST;
    }
    for (auto arg : args())
        os << " " << *arg;
    os << '\n';
}

void Operation::replaceRefsWith(Operation *op) {
    for (size_t i = refs().size()-1; i != (size_t) -1; --i)
        refs()[i]->replace(VRegArg::create(op));
}

void ConstArg::dump(std::ostream &os) const {
    os << YELLOW << value() << RST;
}

void VRegArg::dump(std::ostream &os) const {
    os << BLUE << "%";
    if (ptr()->vregName().empty()) os << ptr();
    else os << ptr()->vregName();
    os << RST;
}

void BlockArg::dump(std::ostream &os) const {
    os << GREEN << "%";
    if (ptr()->name().empty()) os << ptr();
    else os << ptr()->name();
    os << RST;
}

void FuncArg::dump(std::ostream &os) const {
    os << BOLD << "@";
    if (ptr()->name().empty()) os << ptr();
    else os << ptr()->name();
    os << RST;
}

void ArgArg::dump(std::ostream &os) const {
    os << RED << "%";
    if (ptr()->name().empty()) os << ptr();
    else os << ptr()->name();
    os << RST;
}

UPtr<SimpleType> Type::void_t() { return SimpleType::create(T_VOID); }
UPtr<SimpleType> Type::ptr_t() { return SimpleType::create(T_PTR); }
UPtr<SimpleType> Type::i8_t() { return SimpleType::create(T_I8); }
UPtr<SimpleType> Type::i16_t() { return SimpleType::create(T_I16); }
UPtr<SimpleType> Type::i32_t() { return SimpleType::create(T_I32); }
UPtr<SimpleType> Type::i64_t() { return SimpleType::create(T_I64); }

bool Type::isSimple() const {
    return dynamic_cast<const SimpleType*>(this) != nullptr;
}

bool Type::isSimple(SimpleTypeKind kind) const {
    auto st = dynamic_cast<const SimpleType*>(this);
    if (!st) return false;
    return st->kind() == kind;
}

UPtr<Type> SimpleType::clone() const {
    return SimpleType::create(kind());
}

size_t SimpleType::size() const {
    switch (kind()) {
        case T_VOID: return 0;
        case T_I64: return 8;
        case T_I32: return 4;
        case T_I16: return 2;
        case T_I8:  return 1;
        case T_PTR: return 8;
        case T_INVALID: return 1;
    }
    assert(false);
}

size_t SimpleType::align() const {
    switch(kind()) {
        case T_VOID: return 1;
        case T_I64: return 8;
        case T_I32: return 4;
        case T_I16: return 2;
        case T_I8:  return 1;
        case T_PTR: return 8;
        case T_INVALID: return 1;
    }
    assert(false);
}

size_t SimpleType::offset(size_t elem) const {
    return -1;
}

void SimpleType::dump(std::ostream &os) const {
    os << CYAN;
    switch (kind()) {
        case T_VOID: os << "void"; break;
        case T_I64: os << "i64"; break;
        case T_I32: os << "i32"; break;
        case T_I16: os << "i16"; break;
        case T_I8: os << "i8"; break;
        case T_PTR: os << "ptr"; break;
        case T_INVALID: os << "(invalid type)"; break;
    }
    os << RST;
}

bool SimpleType::isSameAs(const Type *t) const {
    auto st = dynamic_cast<const SimpleType*>(t);
    if (!st) return false;
    return st->kind() == kind();
}

SimpleTypeKind SimpleType::kindFromStr(View str) {
    if (str == "void") return T_VOID;
    else if (str == "i64") return T_I64;
    else if (str == "i32") return T_I32;
    else if (str == "i16") return T_I16;
    else if (str == "i8") return T_I8;
    else if (str == "ptr") return T_PTR;
    return T_INVALID;
}

};
