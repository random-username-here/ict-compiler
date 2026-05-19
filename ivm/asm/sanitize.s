///
/// Return checks
///

_ict.sanitize.returnMissing:
    call _ict.sanitize.m_returnMissing, print_cstring
    call quit
    

_ict.sanitize.m_returnMissing: .ascii "PANIC: Sanitizer: function had to, but did not return\n\0"
