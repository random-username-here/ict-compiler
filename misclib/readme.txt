
misclib -- couple of miscelanious pieces

notes:

AST

    Container<Self, Child>
        children : uptr<Child>[]

    Item<Self, Parent>
        parent: Parent*
        
    AnyContainer<Child> : Container<AnyContainer, Child>

    ---

    Test : Item<Test, AnyContainer<Test>>
    A : Container<A, Test>

    A.add(Test())

    A::add(Item<T, P base A>)

    Container

