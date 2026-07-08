#pragma once
#include "./core.h"
#include <cstdint>

namespace ief {

enum SymbolFlags : uint16_t {
    SYMF_PRESENT = 1,
    SYMF_WEAK = 2
};

struct SymbolEntry {
    Size offsetInSection;
    Size size;
    Ref nameOffset;
    uint16_t section;
    SymbolFlags flags;
};

struct Section_sym {
    Size symCount;
    SymbolEntry symbols[];
};

enum RelocFlags : uint16_t {
    RELF_SUB = 1,
    RELF_ADD = 2
};

struct RelocEntry {
    Size offsetInSection;
    Size symIndex;
    uint16_t section;
    RelocFlags flags;
};

struct Section_reloc {
    Size relocCount;
    RelocEntry relocs;
};

};
