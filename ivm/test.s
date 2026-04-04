.func
_start:
    rcall setup_stack
    rcall main
    cli
    hlt

.func
setup_stack:
    push 0x10000
    dup
    ssp
    ssf
    ret

.include "printer.s"
.const __ict.debugPrintInt print_number
.const __ict.debugPrintChar print_char
.include "../build/program.s"

