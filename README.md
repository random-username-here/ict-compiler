# A toy llvm-like compiler

A LLVM-inspired compiler I am currently writting.

It has a SSA IR with functions, blocks, instructions. It has passes. It has analyses.

## Overview of subprojects

Here we have:

 - `misclib/` -- Custom data structures
    - `tree.hpp` -- Trees with strict child/parent typing. Backbone of every AST here.
    - `dump_stream.hpp` -- Wrapper around `std::ostream` for colors/indentation & logging library ontop of it.
    - `array_view.hpp` -- Like `string_view`, but for arrays.
    - ... and some small utilities

 - `modlib/` -- Plugin system

 - `ict/` -- Toy LLVM-inspired compiler toolkit
    - `core/` -- IR data structures, frontend/backend/... interface classes, runner.
    - `analyses/` -- Analyses
        - `below/` -- Which blocks can be reached from that/from which blocks this one can be reached
        - `crossblock/` -- During which blocks vreg must be stored somewhere
        - TODO: `preds/` -- Block predecessor list
    - `passes/` -- Optimization passes
        - TODO: `localize-const/` -- Duplicate crossblock `Const`-s to their destinations
        - TODO: `pat-simplify/` -- Simplifier using pattern matching
        - TODO: `mem-to-reg/` -- Reducing `Alloca` usage
    - `ir/` -- IR reader from files
    - `arch/`
        - `ivm/` -- Stuff for my toy ISA
            - `backend/` -- Backend. Responsible for emitting binary & has `ict/arch/ivm.hpp` with low-level instr names
                TODO: finish writting this
            - `hl2ll/` -- Instruction selection.
            - TODO: `sfplan/` -- Stack frame planner.
            - `opt/`
                - TODO: `alloca2off/` -- Replacing `Alloca` with stack frame offseting
    - `examples/` -- Sample pieces of IR

 - TODO: `icc/` -- C compiler
