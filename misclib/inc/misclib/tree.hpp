/**
 * \file
 * \brief A slot and item library for building AST-like trees, attempt 2
 * \author Didyk Ivan
 * \date 2026-04-10
 * \todo Get some UML editor and think this through another time, this is ugly as hell.
 */
#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

namespace misc {

//==============================================================================
// Utilities
//==============================================================================

template<typename T>
using UPtr = std::unique_ptr<T>;

namespace detail {

/**
 * \brief Class to obtain class type & value type from class member.
 *
 * For example, if we have `Foo { Bar x; }`, `MemberTraits<&Foo::x>`
 * will have `Class = Foo` and `Type = Bar`.
 */
template<auto Value>
struct MemberTraits {};

template<typename ClassT, typename TypeT, TypeT ClassT::* Value>
struct MemberTraits<Value> {
    using Class = ClassT;
    using Type = TypeT;
};

/**
 * Some slots are containers (`SlotList`). Things inheriting from
 * `GenericSlot` are in them. This thing obtains the type of inner thing.
 */
template<typename T>
struct SlotContainerItemT;

template<typename T>
using SlotContainerItem = typename SlotContainerItemT<T>::SlotType;

};

//==============================================================================
// Item
//==============================================================================

template<typename ItemType>
class GenericSlot;

/**
 * \brief Item which only knows a slot it is in.
 * This class is specialized for different kinds of slots later.
 * This item can be added to slots which inherit from that slot type.
 */
template<typename Self, typename Slot>
class ItemBase {
public:
    using SlotType = Slot;
    using ItemType = detail::SlotContainerItem<Slot>;
private:
    template<typename T>
    friend class GenericSlot;
    using ContItem = detail::SlotContainerItem<Slot>;
    ContItem *m_slot = nullptr;
public:
    void m_setSlot(ContItem *s) { m_slot = s; }
    ContItem *slot() { return m_slot; }
    const ContItem *slot() const { return m_slot; }

    template<typename T>
    auto replace(UPtr<T> &&v) {
        return this->slot()->replace(std::move(v));
    }

    auto replace(std::nullptr_t) {
        return this->slot()->replace(nullptr);
    }
};

template<typename Self, typename Slot = GenericSlot<Self>>
class Item : public ItemBase<Self, Slot> {};

//==============================================================================
// One-item slot
//==============================================================================

/**
 * \brief A slot base, which contains one item of specified type.
 *
 * This slot does not known anything about object it is in.
 * This only accepts `Item`-s as template paremater,
 * but does not check, because slots are defined while items
 * are only forward declared.
 *
 * Slots cannot be copied. This is a base class for all other slots.
 *
 * \tparam ItemType     --  Type of items in that slot
 *
 */
template<typename ItemType>
class GenericSlot {
    UPtr<ItemType> m_value;

public:

    UPtr<ItemType> replace(UPtr<ItemType> &&v) {
        if (m_value)
            m_value->m_setSlot(nullptr);
        auto prevVal = std::move(m_value);
        m_value = std::move(v);
        if (m_value)
            m_value->m_setSlot(static_cast<typename ItemType::ItemType*>(this));
        return prevVal;
    }

    GenericSlot() { }
    GenericSlot(UPtr<ItemType> &&v) { replace(std::move(v)); }
    GenericSlot(GenericSlot &&v) 
        { replace(v.replace(nullptr)); }
    GenericSlot &operator=(GenericSlot &&v) 
        { replace(std::move(v)); return *this; }

    ~GenericSlot() 
        { replace(nullptr); }

    ItemType *get() { return this == nullptr ? nullptr : m_value.get(); }
    const ItemType *get() const { return this == nullptr ? nullptr : m_value.get(); }

    ItemType *operator->() { return get(); }
    const ItemType *operator->() const { return get(); }

    ItemType &operator*() { return *get(); }
    const ItemType &operator*() const { return *get(); }

    ItemType *operator=(UPtr<ItemType> &&v) {
        replace(std::move(v));
        return get();
    }
};

/**
 * A basic slot type, which:
 *  - Has no known parent
 *  - Will provide itself as slot type
 */


/**
 * \brief A slot which has only one child item.
 */
template<typename ParentType, typename ElemType>
class Slot : public GenericSlot<ElemType> {
    ParentType *m_parent;
public:
    Slot(ParentType *cont) :m_parent(cont), GenericSlot<ElemType>(nullptr)  {}
    Slot(ParentType *cont, UPtr<ElemType> &&v) :m_parent(cont), GenericSlot<ElemType>(std::move(v)) {}

    ParentType *parent() { return m_parent; }
    const ParentType *parent() const { return m_parent; }
    using GenericSlot<ElemType>::operator=;

    operator bool() const { return this->get() != nullptr; }
};

namespace detail {

template<typename T>
struct SlotContainerItemT<GenericSlot<T>> { using SlotType = GenericSlot<T>; };

template<typename T, typename P>
struct SlotContainerItemT<Slot<P, T>> { using SlotType = Slot<P, T>; };

};

//==============================================================================
// Slot list
//==============================================================================

template<typename ParentType, typename ElemType>
class SlotList {
public:
    class Node;
private:
    UPtr<Node> m_first;
    Node *m_last = nullptr;
    ParentType *m_parent = nullptr;
    size_t m_size = 0;
public:

    SlotList(ParentType *p) :m_parent(p) {}
    SlotList(const SlotList &v) = delete;
    SlotList& operator=(const SlotList &v) = delete;

    template<typename ...Args>
    SlotList(ParentType *p, Args &&...args) :m_parent(p) {
        (insertBefore(nullptr, std::forward<Args>(args)), ...);
    }
    
    size_t size() const { return m_size; }

    class Node : public GenericSlot<ElemType> {
        friend class SlotList;

        SlotList *m_list;
        UPtr<Node> m_next = nullptr;
        Node *m_prev = nullptr;
    public:
        Node(UPtr<ElemType> &&v, SlotList *l) :m_list(l), GenericSlot<ElemType>(std::move(v)) {}

        SlotList *list() { return m_list; }
        const SlotList *list() const { return m_list; }
        Node *prev() { return m_prev; }
        const Node *prev() const { return m_prev; }
        Node *next() { return m_next.get(); }
        const Node *next() const { return m_next.get(); }
        ParentType *parent() { return m_list->m_parent; }
        const ParentType *parent() const { return m_list->m_parent; }
        using GenericSlot<ElemType>::operator=;
    };

    ElemType *first() { return m_first ? m_first->get() : nullptr; }
    ElemType *last() { return m_last ? m_last->get() : nullptr; }

    const ElemType *first() const { return m_first ? m_first->get() : nullptr; }
    const ElemType *last() const { return m_last ? m_last->get() : nullptr; }


private:

    // This will probbably not work with STL algorithms, but I need iterators
    // just to iterate elements in a for loop.
    template<typename NodeType>
    class IteratorBase {
        friend class SlotList;
        NodeType *m_elem;
        IteratorBase(NodeType *n) :m_elem(n) {}
    public:

        auto *operator->() { return **m_elem; }
        const ElemType *operator->() const { return **m_elem; }
        auto *operator*() { return (*m_elem).get(); }
        const ElemType *operator*() const { return (*m_elem).get(); }

        bool operator==(IteratorBase it) const { return it.m_elem == m_elem; }  
        bool operator!=(IteratorBase it) const { return it.m_elem != m_elem; }  

        IteratorBase &operator++() { m_elem = m_elem ? m_elem->next() : nullptr; return *this; }
        IteratorBase &operator--() { m_elem = m_elem ? m_elem->prev() : nullptr; return *this; }

        IteratorBase operator++(int) { auto v = *this; m_elem = m_elem ? m_elem->next() : nullptr; return v; }
        IteratorBase operator--(int) { auto v = *this; m_elem = m_elem ? m_elem->prev() : nullptr; return v; }
    };
public:

    using Iterator = IteratorBase<Node>;
    using ConstIterator = IteratorBase<const Node>;

    Iterator begin() { return Iterator(m_first.get()); }
    Iterator end() { return Iterator(nullptr); }
    ConstIterator begin() const { return ConstIterator(m_first.get()); }
    ConstIterator end() const { return ConstIterator(nullptr); }

    template<typename El = ElemType>
    El *insertAfter(ElemType *child, UPtr<El> &&newOne) {
        auto ptr = newOne.get();
        auto o_new = std::make_unique<Node>(std::move(newOne), this);

        if (child == nullptr) {
            if (m_first) {
                auto o_first = static_cast<UPtr<Node>&&>(std::move(m_first));
                o_first->m_prev = o_new.get();
                o_new->m_next = std::move(o_first);
                m_first = std::move(o_new);
            } else {
                m_first = std::move(o_new);
                m_last = static_cast<Node*>(m_first.get());
            }
        } else {
            auto prev = child->slot();
            o_new->m_prev = prev;
            if (prev->m_next) {
                auto o_next = std::move(prev->m_next);
                o_next->m_prev = o_new.get();
                o_new->m_next = std::move(o_next);
            } else {
                m_last = o_new.get();
            }
            prev->m_next = std::move(o_new);
        }
        m_size++;
        return ptr;
    }

    template<typename El = ElemType, typename ...Args>
    El *createAfter(ElemType *child, Args &&...args) {
        auto o_elem = std::make_unique<El>(std::forward<Args>(args)...);
        auto ptr = o_elem.get();
        insertAfter(child, std::move(o_elem));
        return ptr;
    }

    template<typename El = ElemType>
    El *insertBefore(ElemType *child, UPtr<El> &&e) {
        return insertAfter(child == nullptr ? last() : static_cast<Node*>(child->slot())->prev()->get(), std::move(e));
    }

    template<typename El = ElemType, typename ...Args>
    El *createBefore(ElemType *child, Args &&...args) {
        auto o_elem = std::make_unique<El>(std::forward<Args>(args)...);
        auto ptr = o_elem.get();
        insertBefore(child, std::move(o_elem));
        return ptr;
    }
    
    template<typename El = ElemType>
    El *insertEnd(UPtr<El> &&e) {
        return insertBefore(nullptr, std::move(e));
    }

    template<typename El = ElemType, typename ...Args>
    El *createEnd(Args &&...args) {
        return createBefore(nullptr, std::forward<Args>(args)...);
    }
    
    UPtr<ElemType> extract(ElemType *child) {
        auto node = static_cast<Node*>(child->slot());
        assert(node->m_list == this);
        m_size--;
        if (m_first.get() == node) {
            auto o_first = std::move(m_first);
            m_first = std::move(o_first->m_next);
            if (m_first) m_first->m_prev = nullptr;
            if (m_last == node) m_last = nullptr;
            return o_first->replace(nullptr);
        } else {
            auto o_first = std::move(node->m_prev->m_next);
            node->m_prev->m_next = std::move(node->m_next);
            if (node->m_prev->m_next)
                node->m_prev->m_next->m_prev = node->m_prev;
            else
                m_last = node->m_prev;
            return o_first->replace(nullptr);
        }
    }

    class Inserter {
        SlotList *m_list;
        ElemType *m_before;
    public:
        Inserter(SlotList *l, ElemType *before = nullptr) :m_list(l), m_before(before) {}

        template<typename El = ElemType>
        El *insert(UPtr<El> &&p) { return m_list->insertBefore<El>(m_before, std::move(p)); }

        template<typename El = ElemType, typename ...Args>
        El *create(Args &&...args) { return m_list->createBefore(m_before, std::forward<Args>(args)...); }
    };

    Inserter backInserter() { return Inserter(this, nullptr); }
    Inserter inserterBefore(ElemType *el) { return Inserter(this, el); }
};

namespace detail {

template<typename T, typename P>
struct SlotContainerItemT<SlotList<P, T>> { using SlotType = typename SlotList<P, T>::Node; };

};

//==============================================================================
// Slot vector
//==============================================================================

template<typename ParentType, typename ElemType>
class SlotVector {
public:
    class Entry : public GenericSlot<ElemType> {
        friend class SlotVector;
        SlotVector *m_vec;

    public:
        Entry(UPtr<ElemType> &&v, SlotVector *sv) :m_vec(sv), GenericSlot<ElemType>(std::move(v)) {}

        ParentType *parent() { return m_vec->m_parent; }
        const ParentType *parent() const { return m_vec->m_parent; }
        using GenericSlot<ElemType>::operator=;

    };
private:
    ParentType *m_parent;
    std::vector<Entry> m_entries;
public:

    SlotVector(ParentType *p) :m_parent(p) {}
    SlotVector(const SlotVector &v) = delete;
    SlotVector& operator=(const SlotVector &v) = delete;

    template<typename ...Args>
    SlotVector(ParentType *p, Args &&...args) :m_parent(p) {
        (push(std::forward<Args>(args)), ...);
    }

    ElemType *first() { return m_entries.empty() ? nullptr : m_entries.front().get(); }
    ElemType *last() { return m_entries.empty() ? nullptr : m_entries.back().get(); }

    ElemType *at(size_t i) { return m_entries[i].get(); }
    const ElemType *at(size_t i) const { return m_entries[i].get(); }
    ElemType *operator[](size_t i) { return m_entries[i].get(); }
    const ElemType *operator[](size_t i) const { return m_entries[i].get(); }

    template<typename El = ElemType>
    El *push(UPtr<El> &&v) { m_entries.push_back(Entry(std::move(v), this)); return static_cast<El*>(m_entries.back().get()); }

    template<typename El = ElemType, typename ...Args>
    El *createEnd(Args &&...args) {
        return push<El>(std::make_unique<El>(std::forward<Args>(args)...));
    }

    UPtr<ElemType> pop() {
        auto o_elem = m_entries.back().replace(nullptr);
        m_entries.pop_back();
        return o_elem;
    }


private:

    template<typename BaseType>
    class IteratorBase {
        friend class SlotVector;
        BaseType *m_vec;
        size_t m_pos = 0;
    public:

        IteratorBase(BaseType *vec, size_t pos) :m_vec(vec), m_pos(pos) {}

        auto *operator->() { return m_vec->at(m_pos); }
        const ElemType *operator->() const { return m_vec->at(m_pos); }
        auto *operator*() { return m_vec->at(m_pos); }
        const ElemType *operator*() const { return m_vec->at(m_pos); }

        bool operator==(IteratorBase it) const { return it.m_vec == m_vec && it.m_pos == m_pos; }  
        bool operator!=(IteratorBase it) const { return !(it == *this); }  

        IteratorBase &operator++() { m_pos++; return *this; }
        IteratorBase &operator--() { m_pos--; return *this; }
        IteratorBase operator++(int) { auto v = *this; m_pos++; return v; }
        IteratorBase operator--(int) { auto v = *this; m_pos--; return v; }
    };

public:

    using Iterator = IteratorBase<SlotVector>;
    using ConstIterator = IteratorBase<const SlotVector>;

    size_t size() const { return m_entries.size(); }

    Iterator begin() { return Iterator(this, 0); }
    Iterator end() { return Iterator(this, size()); }
    ConstIterator begin() const { return ConstIterator(this, 0); }
    ConstIterator end() const { return ConstIterator(this, size()); }
};

namespace detail {

template<typename T, typename P>
struct SlotContainerItemT<SlotVector<P, T>> { using SlotType = typename SlotVector<P, T>::Entry; };

};


//==============================================================================
// Item
//==============================================================================

/**
 * \brief Item specialization for cases when slot knows parent.
 */
template<typename Self, typename ItemType, typename ContType>
class Item<Self, Slot<ContType, ItemType>> : public ItemBase<Self, Slot<ContType, ItemType>> {
public:
    ContType* parent() { return this->slot()->parent(); }
    const ContType* parent() const { return this->slot()->parent(); }
};

/**
 * \brief Item specialization for cases when slot knows parent.
 */
template<typename Self, typename ItemType, typename ContType>
class Item<Self, SlotList<ContType, ItemType>> : public ItemBase<Self, SlotList<ContType, ItemType>> {
public:
    ItemType *next() { return this->slot()->next()->get(); }
    const ItemType *next() const { return this->slot()->next()->get(); }

    ItemType *prev() { return this->slot()->prev()->get(); }
    const ItemType *prev() const { return this->slot()->prev()->get(); }

    ContType* parent() { return this->slot()->parent(); }
    const ContType* parent() const { return this->slot()->parent(); }

    SlotList<ContType, ItemType> *parentList() { return this->slot()->list(); }
    const SlotList<ContType, ItemType> *parentList() const { return this->slot()->list(); }

    UPtr<Self> extractSelf() {
        return static_cast<UPtr<Self>>(this->slot()->list()->extract(static_cast<Self*>(this)));
    }
};

template<typename Self, typename ItemType, typename ContType>
class Item<Self, SlotVector<ContType, ItemType>> : public ItemBase<Self, SlotVector<ContType, ItemType>> {
public:
    ContType* parent() { return this->slot() ? this->slot()->parent() : nullptr; }
    const ContType* parent() const { return this->slot() ? this->slot()->parent() : nullptr; }
};

namespace detail {
    template<typename T>
    struct MaybeReturnTypeT { using Type = T; };
    template<typename Rt, typename ...Args>
    struct MaybeReturnTypeT<Rt(Args...)> { using Type = Rt; };
    template<typename Cls, typename Rt, typename ...Args>
    struct MaybeReturnTypeT<Rt(Cls::*)(Args...)> { using Type = Rt; };

    template<typename T>
    using MaybeReturnType = typename MaybeReturnTypeT<T>::Type;

    template<typename T, typename R, typename ...Args>
    constexpr auto non_cv_method(R (T::*x)(Args ...args)) { return x; }
};


#define MISC_ITEM_IN(Self, In) misc::Item<Self, std::remove_reference_t<std::remove_pointer_t<misc::detail::MaybeReturnType<decltype(misc::detail::non_cv_method(In))>>>>

};
