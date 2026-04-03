# TODO list

 - [x] icl-parse -- parsing

 - [ ] icl-expr -- simple math expressions
    - Ablity to define custom functions
    - Has a callback to request variable values

 - [ ] icl-data -- nginx-like data format

 - [ ] icl-asm -- assembler
    - Archtectures
        - [ ] data (not an architecture)
        - [ ] ivm
        - [ ] x86
    - Output formats
        - [ ] bin
        - [ ] elf (section support needed)
    - Good directives
        - [ ] .include
        - [ ] .section
        - [ ] .once
        - [ ] .const
        - [ ] .if, .else, .elif, .endif

 - [ ] icl-mas -- multi-assignement ir
    - Generic IR optimizations
        - [ ] Constant folding
        - [ ] Splitting variables

 - [ ] icl-b-ivm -- backend for ivm

 - [ ] icl-b-x86 -- backend for x86
    - [ ] Use registers 

 - [ ] scl -- Small compiled language
    - [ ] Math
    - [ ] Variables 
    - [ ] Functions
    - [ ] More types

 - [ ] ivm2
    - [ ] Basics
    - [ ] Printer
    - [ ] Interrupts
    - [ ] API for memory-mapped hardware
    - [ ] Registers in low memory
    - [ ] Debug mode
    - [ ] Relative jump instruction for PIC
    - [ ] ELF loading

 - [ ] sgui2 -- gui library
    - [ ] Rewrite the thing
    - [ ] Stacks, min width & height
    - [ ] Widget baseline
    - [ ] Drag & drop
    - [ ] 9slice themes

 - [ ] sgui-editor -- a code editor
    - [ ] Find symbols in current scope
    - [ ] Documentation parsing?
   
 - [ ] sgui-files -- file manager for sgui
    - [ ] Thumbnails
    - [ ] List & icon view
    - [ ] Drag & drop
