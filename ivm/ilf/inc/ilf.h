#pragma once
#include <cstdint>

struct ilf_SectionDecl {
    char name[4];
    uint32_t off, size;
    uint32_t flags;
};

struct ilf_Header {
    int8_t jmp[10];
    uint16_t n_secs;
    struct ilf_SectionDecl secs[];
};

struct ilf_Section_TEXT {
    
};

struct ilf_Section_SYMB {

};

struct ilf_Section_RELA {
    uint32_t
};
