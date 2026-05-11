# `dehumanify` -- make IR more machine-oriented

What this does:

 - Pulls constants from something like `%res Add %x 1` into `Const` operators.
 - Pulls global references from something like `Call @add` into `%add = GlobalPtr @add; Call[i64] %add`
