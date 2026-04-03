# How ivm code is generated

## 

## High level IR -> Low level IR



## Vreg allocations

We spill everything onto second stack. 

## TODO

a proper assembler api, not printing things to ostream directly
multiple sections

## Calling conventions

Arguments are on stack, return address is on top.
Arguments will be eaten by callee.
First argument is on top.

## Stack usage

Function can have some `Alloca`-s. They will be on the stack.
When two paths converge, if they have non-equal alloca counts,
they will be aligned.

Vregs are allocated sequentially. If vreg is not needed after
operation, and is on top, it will be used. If not on top
-- it won't be reused.

Phi-s are not supported yet, TODO: think how to support them later.

Looped alloca will fail
Need stack frame instructions?
 - `fr_begin` & `fr_end`
 - `fr_item N`

```
// some imaginary IR for times far far away
func_impl @abs {
    %arg    Arg 0                   // @-2 (argument)
    %zero   Const 0                 // @0
    %cond   Lt %arg %zero           // pull arg, pull const (todo: reorder pass)
            Br %cond %less %greater 
%less:
    %neg    Neg %arg
            Upsilon %res %neg
            Jmp %end
%greater:
            Upsilon %res %arg
            Jmp %end
%end:
    %res    Phi
            Ret %res
}
```
