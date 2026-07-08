///
/// Return checks
///

_ict.sanitize.returnMissing:
    swap
    call _ict.sanitize.m_returnMissing, print_cstring
    call print_cstring
    call 10, print_char
    call quit
    

_ict.sanitize.m_returnMissing: .ascii "PANIC: Sanitizer: function had to, but did not return from \0"
