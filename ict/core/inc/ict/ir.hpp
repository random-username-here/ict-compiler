/**
 * \brief ICT IR format
 * Here we have Blocks, which have Op-s, which have different
 * arguments.
 * Functions and modules are to be done.
 */
#pragma once
#include "ict/ir-macro.hpp"
#include "misclib/defs.hpp"
#include "misclib/parse.hpp"
#include "refs.hpp"
#include "misclib/tree.hpp"
#include <cstdint>
#include <ostream>

namespace ict {

using misc::UPtr;
using misc::View;

class Arg;
class VRegArg;
class ConstArg;
class Op;
class Block;
class BlockBuilder;
class Function;
class Module;
class Manager;

using Integer = int64_t;

/**
 * \brief Operator's argument
 */
class Arg :
        public misc::Item<Arg, Op>
{
public:
    virtual void dump(std::ostream &os) const = 0;    
    virtual ~Arg() {};
};

/**
 * \brief Argument referring to result of given operator
 */
class VRegArg : 
        public Arg,
        public Ref<VRegArg, Op>
{
public:
    VRegArg() {}
    VRegArg(Op *op) :Ref(op) {}
    
    virtual void dump(std::ostream &os) const override;

    MISC_CREATEFUNC(VRegArg);
};

/**
 * \brief Argument with integer constant
 */
class ConstArg : 
        public Arg
{
    Integer m_value;
public:
    ConstArg() :m_value(0) {}
    ConstArg(Integer v) :m_value(v) {}
   
    Integer value() const { return m_value; }

    virtual void dump(std::ostream &os) const override;
    MISC_CREATEFUNC(ConstArg);
};

class BlockArg :
        public Arg,
        public Ref<BlockArg, Block>
{
public:
    BlockArg() {}
    BlockArg(Block *b) :Ref(b) {}
    virtual void dump(std::ostream &os) const override;
    MISC_CREATEFUNC(BlockArg);
};

/** 
 * \brief Value of `OpKind`. Do not use!
 * Use `OpKind` instead.
 */
namespace OpKinds {
#define X(id, term, vreg, name, textname) constexpr int name = id;
ICT_FOR_ALL_OPS(X)
#undef X
};

using namespace OpKinds;

/**
 * \brief Type of the operator
 */
class OpKind
{
public:
private:
    int m_value;
public:
    OpKind() :m_value(UNKNOWN_OP) {}
    constexpr OpKind(int v) :m_value(v) {}
    constexpr operator int() { return m_value; }

    View name(Manager *mgr = nullptr) const;
    bool doesReturn(Manager *mgr = nullptr) const;
    bool isLowLevel() const { return m_value < 0; }

    static OpKind parse(View name);
};


/**
 * \brief One IR operation
 * Unlike LLVM, here we have only one operation class, with no
 * inheritance.
 *
 * TODO: user-specified output names
 */
class Op :
        public misc::Item<Op, Block>,
        public misc::VarContainer<Op, Arg>,
        public misc::WithToken,
        public Refable<VRegArg>
{

    OpKind m_kind;
    std::string m_vregName;

    // Methods for variadic constructor

    UPtr<Arg>      l_mapToArg(UPtr<Arg> &&a) { return std::move(a); }
    UPtr<ConstArg> l_mapToArg(Integer v) { return ConstArg::create(v); }
    UPtr<VRegArg>  l_mapToArg(Op *op) { return VRegArg::create(op); }
    UPtr<BlockArg> l_mapToArg(Block *block) { return BlockArg::create(block); }
    UPtr<VRegArg>  l_mapToArg(UPtr<Op> &op) { return VRegArg::create(op.get()); }

    void l_pushArg() {}

    template<typename T>
    void l_pushArg(T &&v) {
        push(l_mapToArg(std::forward<T>(v)));
    }

    template<typename First, typename ...Others>
    void l_pushArg(First &&f, Others &&...others) {
        l_pushArg(std::forward<First>(f));
        l_pushArg(std::forward<Others>(others)...);
    }

public:

    OpKind kind() const { return m_kind; }
    View vregName() const { return m_vregName; }
    void setVregName(View v) { m_vregName = v; }

    Op() :m_kind(UNKNOWN_OP) {}

    /**
     * Create operation of given type and arguments.
     * Acceptable arguments are:
     *  - Any `UPtr<Arg>&&` -- they are just added
     *  - Integers -- they are converted to `ConstArg`
     *  - Pointers && `UPtr`-s to `Op`-s -- converted to `VRegArg`
     */
    template<typename ...Args>
    Op(OpKind kind, Args &&...args) {
        m_kind = kind;
        l_pushArg(std::forward<Args>(args)...);
    }

    /**
     * Print operator fancily
     */
    virtual void dump(std::ostream &os) const;

    template<typename T>
    T* arg(size_t n) { return static_cast<T*>(child(n).ptr()); }

    template<typename T>
    const T* arg(size_t n) const { return static_cast<const T*>(child(n).ptr()); }

    ConstArg *carg(size_t n) { return arg<ConstArg>(n); } 
    const ConstArg *carg(size_t n) const { return arg<ConstArg>(n); } 

    VRegArg *varg(size_t n) { return arg<VRegArg>(n); } 
    const VRegArg *varg(size_t n) const { return arg<VRegArg>(n); } 

    BlockArg *barg(size_t n) { return arg<BlockArg>(n); } 
    const BlockArg *barg(size_t n) const { return arg<BlockArg>(n); } 

    /** Replace all references to this Op with another Op */
    void replaceRefsWith(Op *op);
    /** Extract operator from container */
    UPtr<Op> extractSelf();

    MISC_CREATEFUNC(Op);
};

/**
 * \brief A basic block
 */
class Block :
        public misc::VarContainer<Block, Op>,
        public misc::Item<Block, Function>,
        public Refable<BlockArg>
{
    std::string m_name;
public:

    View name() const { return m_name; }
    void setName(View v) { m_name = v; }

    virtual void dump(std::ostream &os) const;

    MISC_CREATEFUNC(Block);
};

/**
 * A class to build operations inside one block
 */
class BlockBuilder {
    size_t m_pos;
    Block *m_block;
public:
    BlockBuilder(Block *block, size_t pos = -1)
        :m_pos(pos == -1 ? block->size() : pos), m_block(block) {}

    BlockBuilder(UPtr<Block> &block, size_t pos = -1)
        :m_pos(pos == -1 ? block->size() : pos), m_block(block.get()) {}

    static BlockBuilder before(Op *op) {
        return BlockBuilder(op->parent(), op->slotInParent());
    }

    Op *insert(UPtr<Op> &&op) {
        Op *ptr = op.get();
        m_block->insert(m_pos, std::move(op));
        m_pos++;
        return ptr;
    }

    template<typename ...Args>
    Op *create(OpKind kind, Args ...args) {
        return insert(Op::create(kind, args...));
    }
};

class Function :
        public misc::VarContainer<Function, Block>,
        public misc::Item<Function, Module>
{
    std::string m_name;
public:
    Function() = default;
    Function(View name) :m_name(name) {}

    View name() const { return m_name; }
    
    virtual void dump(std::ostream &os) const;
    
    MISC_CREATEFUNC(Function);
};


class Module :
        public misc::VarContainer<Module, Function>
{
    friend class Manager;
    Manager *m_mgr;
public:
    virtual void dump(std::ostream &os) const;
    Manager *manager() const { return m_mgr; }

    MISC_CREATEFUNC(Module);
};

}
