# `ict.analyses.crossblock` -- Finding vregs which travel across blocks

## Requirements

 - `ict.analyses.below` for `above`/`below`.

## What?

Check out this example:

```
func_impl @demo
{
%b0:
    %one    Const 1;
            Br %b1;
%b1:
            Br %b2;
%b2:
            DebugPrint %one;
}
```

Here `%one` is defined in `%b0` and used in `%b2`. To achive
that, it must live somewhere through `%b1`.

This analysis finds such vregs and blocks they travel through.

## How?

For each vreg we have some uses. Let's write block being above as `> / >= / < / <=`.

 - `o ∈ incoming(B) : def(o) > B and B >= some use(o)`
 - `o ∈ outgoing(B) : def(o) >= B and B > some use(o)`


