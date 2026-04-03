# `ict.ivm` -- IVM code generation

## Structure

`backend/` holds the actual backend and the architecture header (`ict/arch/ivm.hpp`).

## Lower-level IR (todo)

Here actual operations are selected. We have operations:
 - `IVM.Push %reg` -- push that register onto stack
 - `IVM.R.*` -- real instructions, like `add`
 - `%reg IVM.Pop` -- get top value from stack.

When converting high-level IR to low-level, simple ops are expanded like this:
```
// Original
%res Add %a %b
// After 
IVM.Push %a
IVM.Push %b
IVM.R.Add
%res IVM.Pop
```
