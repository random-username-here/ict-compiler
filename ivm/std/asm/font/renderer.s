///
/// Vectorized font renderer
/// Reads 2-byte commands,
///

///=========================================================
/// CRT routines
///=========================================================

.const CRT_ADDR_X   0x00130
.const CRT_ADDR_Y   0x00132
.const CRT_ADDR_CMD 0x00134

///
/// Paint on CRT.
/// Moves ray from current position to (X, Y),
/// drawing line/not drawing line based on ENABLE.
///
/// In: [y] [x] [enable] [ret]
/// Out:  none
///
.func
crt_cmd:
    fswap 2 // [y] <-> [ret]
    put16 CRT_ADDR_Y
    swap // [enable] <-> [x]
    put16 CRT_ADDR_X
    put8 CRT_ADDR_CMD
    ret


///=========================================================
/// Font interpretation
///=========================================================
/// Letter program consists of some number of commands.
/// Each command is 16-bit wide.
/// First byte means operation: 1 for move, 2 for draw, 0 end.
/// Second byte uses high nibble for X, lower nibble for Y
///
/// Font program has character lookup table, to each char there is
/// int64 offset to its program.

///
/// Draws one letter of the font
///
/// In: [y] [x] [hs] [ws] [letter] [ret]
/// Out: none
///
.func
crt_char:
    spush
    sfbegin
    spush // letter @ -8
    rsh 4 // divide by 16, so width is in same units as x/y
    spush // ws     @ -16
    rsh 4 // same
    spush // hs     @ -24
    spush // x      @ -32
    spush // y      @ -40

    sfload64 -8
    mul 8
    add font
    get64
    add font

_crt_char_loop:
    // [cmd addr]
    dup
    get16
    // [addr] [cmd]

    // get operation
    dup
    rsh 8
    and 0xff

    // check if it is letter end? (=0)
    dup
    iszero
    rjnz _crt_char_end

    // else convert it to crt enable: 1 -> 0 (move), 2 -> 1 (draw)
    sub 1
    // [addr] [cmd] [mode]
    swap
    // get x
    dup
    rsh 4
    and 0xf
    sfload64 -16
    mul
    sfload64 -32
    add
    // [addr] [mode] [cmd] [x]
    // now y
    swap
    and 0xf
    sfload64 -24
    mul
    sfload64 -40
    add
    // [addr] [mode] [x] [y]
    fswap 1
    rcall crt_cmd
    // advance addr
    add 2
    rjmp _crt_char_loop

_crt_char_end:
    drop
    drop
    drop
    sfend
    spop
    ret


