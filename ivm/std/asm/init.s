///
/// init.s -- Initialization, _start
///

.func
_start:
    // Setup stack
    ssp 0x10000
    ssf 0x10000
    
    // Start program
    rcall main

    // Quit VM
    cli
    hlt
