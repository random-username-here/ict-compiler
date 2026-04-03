#pragma once
/**
 * \brief ANSI terminal escape sequences
 */
namespace ansi {
    static const char *RST      = "\x1b[0m";
    static const char *BOLD     = "\x1b[1m";
    
    static const char *LGRAY    = "\x1b[37m";
    static const char *DGRAY    = "\x1b[90m";
    
    static const char *RED      = "\x1b[91m";
    static const char *GREEN    = "\x1b[92m";
    static const char *YELLOW   = "\x1b[93m";
    static const char *BLUE     = "\x1b[94m";
    static const char *PURPLE   = "\x1b[95m";
    static const char *CYAN     = "\x1b[96m";
};
