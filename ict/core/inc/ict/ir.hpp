/**
 * \brief ICT IR format
 * Here we have Blocks, which have Op-s, which have different
 * arguments. Op's can have their types explicitly specified.
 * Functions and modules are to be done.
 */
#pragma once
#include "ict/ir-macro.hpp"
#include "misclib/defs.hpp"
#include "refs.hpp"
#include "misclib/tree.hpp"
#include <cassert>
#include <cstdint>
#include <ostream>
#include <utility>

namespace ict {

using misc::UPtr;
using misc::View;

class Manager;      ///< Manages passes, there is usually one manager, not here
class Module;       ///< One compilation unit (file)

class FunctionDecl; ///< Function declaration, with argument types, etc.
class ArgDecl;      ///< Named argument in function declaration

class FunctionImpl; ///< Function implementation.
class BasicBlock;   ///< Basic block
class Operation;    ///< Operation (aka instruction)

class Arg;          ///< Any argument of operator
class VRegArg;      ///< Argument pointing to virtual register
class ConstArg;     ///< Argument with constant integer value
class BlockArg;     ///< Argument pointing to some block
class FuncArg;      ///< Argument pointing to some function
class ArgArg;       ///< Argument pointing to function's argument

class Type;         ///< Some IR type 
class SimpleType;   ///< Basic types -- ints, pointers (they are type-less), ...
class ArrayType;    ///< Fixed-size array

using Integer = int64_t;

/**
 * Module -- one compiled file.
 */
class Module
{
    friend class Manager;
    std::string m_name;
    misc::SlotList<Module, FunctionDecl> m_funcDecls;
    misc::SlotList<Module, FunctionImpl> m_funcImpls;
public:

    Module(View name = "unnamed") :m_funcDecls(this), m_funcImpls(this), m_name(name) {}

    auto &funcDecls() { return m_funcDecls; }
    auto &funcImpls() { return m_funcImpls; }
    auto &funcDecls() const { return m_funcDecls; }
    auto &funcImpls() const { return m_funcImpls; }
    View name() const { return m_name; }

    /** Find function with given name */
    FunctionDecl *findFunc(View name);

    /**
     * Finds/declares function with given signature.
     * Returns null if it cannot be done (name is taken by someone other).
     */
    template<typename ...Args>
    FunctionDecl *findOrDeclareFunc(View name, UPtr<Type> &&rt, Args &&...args);

    /** Check if module has correct code */
    bool verify() const;

    virtual void dump(std::ostream &os) const;
    MISC_CREATEFUNC(Module);
};

/**
 * Function declaration -- like `func_decl int64 @add(int64 %a, int64 %b)`.
 */
class FunctionDecl :
        public MISC_ITEM_IN(FunctionDecl, &Module::funcDecls),
        public Refable<FuncArg> 
{
    friend class FunctionImpl;
    FunctionImpl *m_impl = nullptr;
    std::string m_name;
    misc::SlotVector<FunctionDecl, ArgDecl> m_args;
    misc::Slot<FunctionDecl, Type> m_retType;

    UPtr<ArgDecl> l_toArg(UPtr<ArgDecl> &&v) { return std::move(v); }
    UPtr<ArgDecl> l_toArg(UPtr<Type> &&t);

public:

    FunctionDecl() :m_name(""), m_args(this), m_retType(this) {}

    template<typename ...Args>
    FunctionDecl(View name, UPtr<Type> &&rt, Args &&...args) :m_name(name), m_args(this), m_retType(this) {
        m_retType.replace(std::move(rt));
        (m_args.push(l_toArg(std::move(args))), ...);
    }

    View name() const { return m_name; }
    void setName(View n) { m_name = n; }
    auto &args() { return m_args; }
    const auto &args() const { return m_args; }
    auto &retType() { return m_retType; }
    const auto &retType() const { return m_retType; }

    /**
     * Creates function implementation. Returns nullptr if implementation
     * already exists.
     */
    FunctionImpl *implement();

    FunctionImpl *impl() { return m_impl; }
    const FunctionImpl *impl() const { return m_impl; }
    void dump(std::ostream &os) const;
    MISC_CREATEFUNC(FunctionDecl);
};

class ArgDecl :
        public MISC_ITEM_IN(ArgDecl, &FunctionDecl::args),
        public Refable<ArgArg>
{

    std::string m_name;
    misc::Slot<ArgDecl, Type> m_type;

public:

    ArgDecl(UPtr<Type> &&v, View n = "") :m_name(n), m_type(this, std::move(v)) {}

    View name() const { return m_name; }
    void setName(View n) { m_name = n; }

    auto &type() { return m_type; }
    const auto &type() const { return m_type; }

    void dump(std::ostream &os) const;
    MISC_CREATEFUNC(ArgDecl);
};

/**
 * Function implementation, must be attached to some declaration.
 */
class FunctionImpl :
        public MISC_ITEM_IN(FunctionImpl, &Module::funcImpls)
{
    FunctionDecl *m_decl = nullptr;
    misc::SlotList<FunctionImpl, BasicBlock> m_blocks;

public:

    FunctionImpl(FunctionDecl *decl) :m_decl(decl), m_blocks(this) {
        assert(decl);
        assert(decl->m_impl == nullptr);
        decl->m_impl = this;
    }
    
    BasicBlock *createBlock(View name);

    FunctionDecl *decl() { return m_decl; }
    const FunctionDecl *decl() const { return m_decl; }
    auto &blocks() { return m_blocks; }
    const auto &blocks() const { return m_blocks; }

    void dump(std::ostream &os) const;
    MISC_CREATEFUNC(FunctionImpl);
};

/**
 * A basic block inside function implementation.
 * Last operation in basic block must be a terminal instruction!
 */
class BasicBlock :
        public Refable<BlockArg>,
        public MISC_ITEM_IN(BasicBlock, &FunctionImpl::blocks)
{
    std::string m_name;
    misc::SlotList<BasicBlock, Operation> m_operations;
public:

    BasicBlock(View name = "") :m_name(name), m_operations(this) {}

    View name() const { return m_name; }
    void setName(View n) { m_name = std::string(n); }

    auto &operations() { return m_operations; }
    const auto &operations() const { return m_operations; }

    std::vector<const BasicBlock*> predecessors() const;
    std::vector<const BasicBlock*> successors() const;
    std::vector<BasicBlock*> predecessors();
    std::vector<BasicBlock*> successors();

    void dump(std::ostream &os) const;
    MISC_CREATEFUNC(BasicBlock);
};

#define X(id, term, vreg, name, textname) constexpr int OP_##name = id;
ICT_FOR_ALL_OPS(X)
#undef X

/**
 * \brief One action, which may return something.
 *
 * Unlike LLVM, which has a whole tree of various operations, here we have a
 * single operation class. This is not as flexible, but way simpler.
 *
 * Operations usually look like this:
 *      
 *      %sum Add[int] %a %b
 *
 * Operation kind is an integer. Values < 0 are for machine-level IR,
 * values above 0 are defined as OP_... here.
 *
 */
class Operation :
        public Refable<VRegArg>,
        public MISC_ITEM_IN(Operation, &BasicBlock::operations)
{
    int m_kind = 0;
    misc::SlotVector<Operation, Arg> m_args;
    misc::Slot<Operation, Type> m_tparam;
    misc::Slot<Operation, Type> m_retType; // this is initialized using m_args, so it must be later
    std::string m_vregName = "";

    UPtr<Arg>      l_mapToArg(UPtr<Arg> &&a) { return std::move(a); }
    UPtr<ConstArg> l_mapToArg(Integer i);
    UPtr<VRegArg>  l_mapToArg(Operation *o);
    UPtr<BlockArg> l_mapToArg(BasicBlock *b);
    UPtr<FuncArg>  l_mapToArg(FunctionDecl *f);
    UPtr<ArgArg>   l_mapToArg(ArgDecl *a);

    UPtr<Type> l_createRt();

public:

    explicit Operation() :m_kind(OP_UNKNOWN_OP), m_tparam(this), m_args(this), m_retType(this) {}

    /** Generate return type if we did not do that during creation */
    void genReturnType() { m_retType = l_createRt(); }

    explicit Operation(int kind) :
        m_kind(kind),
        m_tparam(this),
        m_args(this),
        m_retType(this, l_createRt()) 
    {}

    template<typename ...Args>
    Operation(int kind, UPtr<Type> &&type, Args &&...args) :
        m_kind(kind),
        m_tparam(this, std::move(type)),
        m_args(this, l_mapToArg(std::forward<Args>(args))...),
        m_retType(this, l_createRt())
    {}
 
    int kind() const { return m_kind; }
    void setKind(int kind) { m_kind = kind; }
    /** Operation kind to string. */
    View name() const;
    View vregName() const { return m_vregName; }
    void setVregName(View n) { m_vregName = n; }
    auto &args() { return m_args; }
    const auto &args() const { return m_args; }

    auto &tparam() { return m_tparam; }
    const auto &tparam() const { return m_tparam; }

    const auto &returnType() const { return m_retType; }

    Arg         *arg(size_t i) { return m_args.at(i); }
    VRegArg     *arg_v(size_t i);
    ConstArg    *arg_c(size_t i);
    BlockArg    *arg_b(size_t i);
    FuncArg     *arg_f(size_t i);
    ArgArg      *arg_a(size_t i);

    const Arg         *arg(size_t i) const { return m_args.at(i); }
    const VRegArg     *arg_v(size_t i) const;
    const ConstArg    *arg_c(size_t i) const;
    const BlockArg    *arg_b(size_t i) const;
    const FuncArg     *arg_f(size_t i) const;
    const ArgArg      *arg_a(size_t i) const;

    /** Is this operation from low-level IR? */
    bool isLowLevel() const { return m_kind < 0; }

    /** 
     * Is the operation terminal?
     * If yes, all arguments which are BlockArg-s must be to it's successors,
     * this is somewhat a hack to make getting predecessors/successors easier.
     */
    bool isTerminal() const;

    bool requiresExplicitType() const;

    static int kindFromString(Manager *mgr, View name);
   
    bool verifyArgsOnly() const;

    void replaceRefsWith(Operation *op);

    void dump(std::ostream &os) const;
    MISC_CREATEFUNC(Operation);
};

class Arg : 
        public MISC_ITEM_IN(Arg, &Operation::args)
{
public:
    virtual void dump(std::ostream &os) const = 0;    
    virtual ~Arg() {};
};

class ConstArg :
        public Arg 
{
    Integer m_value;
public:

    ConstArg() :m_value(0) {}
    ConstArg(Integer v) :m_value(v) {}

    Integer value() const { return m_value; }
    void setValue(Integer v) { m_value = v; }

    void dump(std::ostream &os) const override;
    
    MISC_CREATEFUNC(ConstArg);
};

class VRegArg : 
        public Arg,
        public Ref<VRegArg, Operation> 
{
public:
    using Ref::Ref;
    void dump(std::ostream &os) const override;
    MISC_CREATEFUNC(VRegArg);
};

class BlockArg :
        public Arg,
        public Ref<BlockArg, BasicBlock>
{
public:
    using Ref::Ref;
    void dump(std::ostream &os) const override;
    MISC_CREATEFUNC(BlockArg);
};

class FuncArg :
        public Arg,
        public Ref<FuncArg, FunctionDecl>
{
public:
    using Ref::Ref;
    void dump(std::ostream &os) const override;
    MISC_CREATEFUNC(FuncArg);
};

class ArgArg :
        public Arg,
        public Ref<ArgArg, ArgDecl>
{
public:
    using Ref::Ref;
    void dump(std::ostream &os) const override;
    MISC_CREATEFUNC(ArgArg);
};

enum SimpleTypeKind {
    T_INVALID = 0, T_VOID, T_I64, T_I32, T_I16, T_I8, T_PTR
};

class Type : 
        public misc::Item<Type> {
public:
    virtual UPtr<Type> clone() const = 0;
    virtual size_t size() const = 0;
    virtual size_t align() const = 0;
    virtual size_t offset(size_t elem) const = 0; // -1 if no item
    virtual void dump(std::ostream &os) const = 0;

    static UPtr<SimpleType> void_t();
    static UPtr<SimpleType> ptr_t();
    static UPtr<SimpleType> i8_t();
    static UPtr<SimpleType> i16_t();
    static UPtr<SimpleType> i32_t();
    static UPtr<SimpleType> i64_t();
    //static UPtr<ArrayType> array_t(size_t n, UPtr<Type> &&elem);

    bool isSimple() const;
    bool isSimple(SimpleTypeKind kind) const;
    bool isVoid() const { return isSimple(T_VOID); }
    //bool isArray() const;
    virtual bool isSameAs(const Type *t) const = 0;
};

/**
 * \brief A primitive type 
 * Do not construct explicitly, use `Type::s_...`
 */
class SimpleType : 
        public Type 
{
    SimpleTypeKind m_kind;
public:

    SimpleType(SimpleTypeKind kind) :m_kind(kind) {}

    SimpleTypeKind kind() const { return m_kind; }
    UPtr<Type> clone() const override;
    size_t size() const override;
    size_t align() const override;
    size_t offset(size_t elem) const override;
    void dump(std::ostream &os) const override;
    virtual bool isSameAs(const Type *t) const override;

    static SimpleTypeKind kindFromStr(View str);

    MISC_CREATEFUNC(SimpleType);
};

// TODO
/*
class ArrayType : 
        public Type 
{
public:
    size_t count;
    misc::Slot<ArrayType, Type> type;

    ArrayType(size_t count, UPtr<Type> &&elem) :count(count), type(this, std::move(elem)) {}

    size_t alignedItemSize() const;

    UPtr<Type> clone() const override;
    size_t size() const override;
    size_t align() const override;
    size_t offset(size_t elem) const override;

    void dump(std::ostream &os) const override;
    MISC_CREATEFUNC(SimpleType);
};*/

template<typename ...Args>
FunctionDecl *Module::findOrDeclareFunc(View name, UPtr<Type> &&rt, Args &&...args) {
    auto existing = findFunc(name);
    if (existing) {
        if (!existing->retType()->isSameAs(rt.get()))
            return nullptr;
        if (existing->args().size() != sizeof...(args))
            return nullptr;
        UPtr<ArgDecl> o_args[] = { std::forward<Args>(args)... };
        for (size_t i = 0; i < existing->args().size(); ++i)
            if (!o_args[i]->type()->isSameAs(existing->args().at(i)->type().get()))
                return nullptr;
        return existing;
    } else {
        return funcDecls().createBefore(nullptr, name, std::move(rt), std::forward<Args>(args)...);
    }
}

};
