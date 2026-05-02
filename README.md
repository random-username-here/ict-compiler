# A toy llvm-like compiler

A LLVM-inspired compiler I am currently writting.

It has a SSA IR with functions, blocks, instructions. It has passes. It has analyses.

## Overview of subprojects

Here we have:

 - `misclib/` -- Custom data structures
    - `tree.hpp` -- Trees with strict child/parent typing. Backbone of every AST here.
    - `dump_stream.hpp` -- Wrapper around `std::ostream` for colors/indentation & logging library ontop of it.
    - `array_view.hpp` -- Like `string_view`, but for arrays. I probbably should remove/replace it with `std::span`.
    - ... and some small utilities

 - `modlib/` -- Plugin system TODO: upgrade it to bardak's modlib

 - `ict/` -- Toy LLVM-inspired compiler toolkit
    - `core/` -- IR data structures, frontend/backend/... interface classes, runner. TODO: Decouple Manager from Module
    - `analyses/` -- Analyses. TODO: I should redo their interface, so they are per-function.
        - There were `crossblock` (in which blocks vreg must exist) && `above` (which block is reachable from which),
          which were meant for smarter vreg allocator, but were removed after big IR structure rewrite. 
        - TODO: `domination/` -- Block domination
        - TODO: `loop-tree/` -- Finding loops, I experimented with that algorithm in other repo
    - `passes/` -- Optimization passes
        - TODO: `localize-const/` -- Duplicate crossblock `Const`-s to their destinations
        - TODO: `pat-simplify/` -- Simplifier using pattern matching
    - `ir/` -- IR reader from files
    - `arch/`
        - `ivm/` -- Stuff for my toy ISA
            - `backend/` -- Backend. Defines low-level ops in `ict/arch/ivm.hpp`, and contains functions to emit them.
            - `hl2ll/` -- Instruction selection.
            - `basic-stackplan/` -- "register" allocator, which just spills everything onto stack.
            - `opt/`
                - TODO!!!: `simple-reorder/` -- Reorder operations in one block to remove constant `ToStack`/`FromStack` movement
                - TODO: `alloca2off/` -- Replacing `Alloca` with stack frame offseting
    - `examples/` -- Sample pieces of IR

 - `scl/` -- Simple compiled language
 - `ivm/` -- Pieces for running compiled IVM programs

TODO: write readmes

## Architecture

This thing is highly modular. Each component is compiled into `.so` library,
which are then loaded by the main program.

There are 4 types of components:

 - **Frontend** -- something reading input file and converting it into SSA IR
 - **Backend** -- something taking IR in and emitting it into string
 - **Analysis** -- something which can be ran on IR to provide some insignts.
 - **Pass** -- something running on IR which changes it

Frontend is chosen based on input file name -- if frontend plugin returns
`true` from `takesFile()`, he will be used.

Backend is chosen by arch ID. Currently there is only one arch -- `ivm`,
and chosing it is hardwired into main program.

Passes have `int order()`, by which they are sorted. Pass with lower order
is ran first.

By convention, passes with order < 0 are ran before instruction selection,
passes with order > 0 after, and passes with order > 100 after vreg allocation.

Pass with order = 0 is the instruction selection pass, pass with order = 100 is
vreg allocator.

## Compiling & running sample

This requires
 - `cmake` for building
 - `iasm` & `ivm` for compiling & running resulting assembly (compile them from [https://github.com/random-username-here/mipt-ded-vm])
 - Those in turn require `raylib` & `meson` for being built

Compile it like this:
```sh
$ git clone git@github.com:random-username-here/ict-compiler.git
$ cd ict-compiler
$ cmake -B build
$ cd build
$ make
```

Here is how to run an example program:

```sh
# Compile one of example files
# Will output program.s
$ ./ict/core/ict ../ict/examples/loop.ict
# Assemble resulting binary
# Uses runtime written in that file, it includes `program.s`
$ ./iasm ../ivm/test.s test.bin
# Run
$ ./ivm test.bin
```

I will work on attaching assembler & maybe adding linking to attach runtime later,
so for now this is the way.
