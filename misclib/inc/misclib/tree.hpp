/**
 * \file
 * \brief Slot-based AST trees
 *
 * This design is somewhat simular to LLVM's, except
 * LLVM has child items in linked lists.
 */
#pragma once
#include "misclib/array_view.hpp"
#include "defs.hpp"
#include <vector>
#include <array>
#include <cassert>

namespace misc {

template<typename Child>
class Container;
template<typename Self, typename Child>
class StrictContainer;
template<typename Self, typename Parent>
class Item;

//------------------------------------------------------------------------------
// Base container
//------------------------------------------------------------------------------

/**
 * \brief A container.
 *
 * It contains an array of children. How array is stored
 * (is that a dynamic array or static one) is up to
 * inheriting class.
 *
 * Children can be acessed by index and replaced.
 *
 * This variant has child's parent links of generic `Container<Child>`
 * type. To have them of your type (so, for example, `Function::parent()` is
 * `Module`), use `StrictContainer<Self, Child>`.
 *
 */
template<typename Child>
class Container {

public:    
    using ChildType = Child;

                              /*----<//o//>----*/

    // Methods intended to be overriden

    /// Array of node's children
    virtual ArrayView<const UPtr<Child>> children() const = 0;
    virtual ArrayView<UPtr<Child>> children() = 0;

    virtual ~Container() {};

                              /*----<//o//>----*/
    
    // Helpers
    
    UPtr<Child> replaceChild(size_t slot, UPtr<Child> &&child) {
        UPtr<Child> prev = std::move(children()[slot]);
        if (prev)
            p_dettachChild(prev.get(), slot);
        if (child)
            p_attachChild(child.get(), slot);
        children()[slot] = std::move(child);
        return prev;
    }

    UPtr<Child> *begin() { return children().begin(); }
    UPtr<Child> *end() { return children().end(); }

    const UPtr<Child> *begin() const { return children().begin(); }
    const UPtr<Child> *end() const { return children().end(); }

    size_t size() const { return children().size(); }

                                  /*-----<<~>>-----*/

    // SlotProxy and [] operator

    // in non-const case we either assign value via replaceChild(),
    // or decay into reference.


    /**
     * \brief Version of `p_SlotProxy` for cases when container is constant.
     */
    class ConstSlotProxy {
        friend class Container<Child>;
        const Container<Child> &to;
        size_t pos;
        
        ConstSlotProxy(const Container<Child> &cls, size_t idx)
            :to(cls), pos(idx) {}

        ConstSlotProxy() = delete;
    public:
        const Child* operator->() const { return to.children()[pos].get(); }
        const Child* ptr() const { return to.children()[pos].get(); }
        const UPtr<Child> &uptr() const { return to.children()[pos]; }
        const Child &operator*() const { return *to.children()[pos]; }

        operator const Child*() const { return to.children()[pos].get(); }
    };

    /**
     * \brief A special object, assignment to which will replace child
     */ 
    class SlotProxy {
        // TODO: templated proxy for something like this:
        //       `MISC_SPEC_SLOT(label, 0, Label)`
        //       (In 0'th slot only can be `Label`)
        friend class Container<Child>;
        Container<Child> &to;
        size_t pos;
        
        SlotProxy(Container<Child> &cls, size_t idx)
            :to(cls), pos(idx) {}

        SlotProxy() = delete;
    public:
        UPtr<Child> operator=(UPtr<Child> &&child) {
            return to.replaceChild(pos, std::move(child));
        }

        Child* operator->() { return to.children()[pos].get(); }
        const Child* operator->() const { return to.children()[pos].get(); }

        Child* ptr() { return to.children()[pos].get(); }
        const Child* ptr() const { return to.children()[pos].get(); }

        UPtr<Child> &uptr() { return to.children()[pos]; }
        const UPtr<Child> &uptr() const { return to.children()[pos]; }

        Child &operator*() { return *to.children()[pos]; }
        const Child &operator*() const { return *to.children()[pos]; }

        operator ConstSlotProxy() {
            return ConstSlotProxy(to, pos);
        }
        
        operator Child*() { return to.children()[pos].get(); }
    };

    ConstSlotProxy child(size_t i) const {
        return ConstSlotProxy(*this, i);
    }
    SlotProxy child(size_t i) {
        return SlotProxy(*this, i);
    }

    ConstSlotProxy operator[](size_t i) const { return child(i); }
    SlotProxy operator[](size_t i) { return child(i); }

protected:

                              /*-----<<~>>-----*/

    // Manual attachChild & dettachChild
    // For cases when we are push-ing children
    // These methods are overriden in `StrictContainer`

    virtual void p_attachChild(Child *ch, size_t slot) = 0;
    virtual void p_dettachChild(Child *ch, size_t slot) = 0;

};

/**
 * \brief Container where child's parent links are strongly typed.
 */
template<typename Self, typename Child>
class StrictContainer : public Container<Child> {

protected:
    virtual void p_attachChild(Child *ch, size_t slot) {
        ch->m_attach(static_cast<Self*>(this), slot);
    }

    virtual void p_dettachChild(Child *ch, size_t slot) {
        ch->m_dettach(static_cast<Self*>(this), slot);
    }
};

//------------------------------------------------------------------------------
// Various container types
//------------------------------------------------------------------------------

/**
 * \brief A container with fixed count of child nodes.
 *
 * For example, binary operators will inherit this.
 */
template<typename Self, typename Child, size_t n>
class FixedContainer : public StrictContainer<Self, Child> {
    std::array<UPtr<Child>, n> m_children;
public:
    virtual ArrayView<const UPtr<Child>> children() const override { return m_children; }
    virtual ArrayView<UPtr<Child>> children() override { return m_children; }
};

/**
 * \brief A container with items in `std::vector`
 *
 * For high-child-count nodes, like Module-s.
 */
template<typename Self, typename Child>
class VarContainer : public StrictContainer<Self, Child> {
    std::vector<UPtr<Child>> m_children;
public:
    virtual ArrayView<const UPtr<Child>> children() const override { return m_children; }
    virtual ArrayView<UPtr<Child>> children() override { return m_children; }

    auto push(UPtr<Child> &&ch) {
        m_children.push_back(std::move(ch));
        this->p_attachChild(m_children.back().get(), m_children.size()-1);
        return this->child(m_children.size()-1);
    }

    auto insert(size_t at, UPtr<Child> &&ch) {
        for (size_t i = at; i < m_children.size(); ++i)
            this->p_dettachChild(m_children[i].get(), i);
        m_children.insert(m_children.begin() + at, std::move(ch));
        for (size_t i = at; i < m_children.size(); ++i)
            this->p_attachChild(m_children[i].get(), i);
        return this->child(at);
    }

    UPtr<Child> extract(size_t at) {
        for (size_t i = at; i < m_children.size(); ++i)
            this->p_dettachChild(m_children[i].get(), i);
        auto v = std::move(m_children[at]);
        m_children.erase(m_children.begin() + at);
        for (size_t i = at; i < m_children.size(); ++i)
            this->p_attachChild(m_children[i].get(), i);
        return v;
    }

    UPtr<Child> pop() {
        this->p_dettachChild(m_children.back().get(), m_children.size()-1);
        UPtr<Child> rv = std::move(m_children.back());
        m_children.pop_back();
        return rv;
    }

};

//------------------------------------------------------------------------------
// Container item
//------------------------------------------------------------------------------

/**
 * \brief Item in one of AST containers
 */
template<typename Self, typename Parent = Container<Self>>
class Item {
    // see Container why
    //static_assert(IsExactSpecialization<Parent, Container>, "Parent of Item must subclass Container");
    //static_assert(std::is_base_of_v<Self, typename Parent::ChildType>, "Child item must be placeable into Parent");

    template<typename Child>
    friend class Container;
    template<typename HSelf, typename Child>
    friend class StrictContainer;

    Parent *m_parent = nullptr;
    size_t m_slot;

    void m_attach(Parent *parent, size_t slot) {
        assert(m_parent == nullptr);
        m_parent = parent;
        m_slot = slot;
    }

    void m_dettach(Parent *parent, size_t slot) {
        assert(m_parent == parent);
        assert(m_slot == slot);
        m_parent = nullptr;
        m_slot = -1;
    }

public:
    using ParentType = Parent;

    Parent *parent() const { return m_parent; }
    size_t slotInParent() const { return m_slot; }

    UPtr<Self> replaceWith(UPtr<Self> &&val) {
        assert(parent() != nullptr);
        return static_cast<UPtr<Self>>(parent()->replaceChild(slotInParent(), std::move(val)));
    } 
};

};

/**
 * Declare an AST slot with given name.
 * Expands to getter & const getter for child in that slot.
 */
#define MISC_SLOT(name, id)\
    SlotProxy name() { return child(id); }\
    ConstSlotProxy name() const { return child(id); }    

/**
 * Declare `::create()` function.
 */
#define MISC_CREATEFUNC(cls)\
    template<typename ...Args>\
    static misc::UPtr<cls> create(Args ...args) {\
        return std::make_unique<cls>(std::forward<decltype(args)>(args)...);\
    }

