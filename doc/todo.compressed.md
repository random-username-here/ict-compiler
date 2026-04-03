The language
    no x86, just vm
    simple
    start with math exprs
    add sort of ssa
    vars, stack scope
    if ... else
    while
    functions: have N nums in, num out
    packs, aka structs

lang
    grammatics:
        exprs
        {  } returns last term
        if (expr) expr else expr
        while (expr) expr
        type name(type arg, type arg, ...) expr
    types:
        iNN
        *type



ir
    Printed as tables
    Numbers with $, locals with %, globals @
    No branching for now
    No typing for now also

    var_def @counter { int }
    var_data @counter {
                Const<int> 0;
    }

    func_def @foo { int; int }

    func_impl @foo {
        // With future typing syntax
        // Last `;` optional
        %a      Arg<int> 0;
        %b      Arg<int> 1;
        %0      Const<int> 42;
        %1      Mul %a, %0;
        %2      Add %1, %b;
                Ret %2;
    }

    impl @other {
        %a      Const<int> 1;
        %res    Call @foo, %a, %b;

    }

opt passes
    IVM stack slots: replace Alloca/Load/Store with Const/Sfload/sfstore
    Constants: reduce const expressions
    In-block Mem2reg: alloca used in one block is moved to stack
