///
/// \file 
/// \brief IVM executable format
///
#pragma once
#include <cstdint>
namespace ief {

using Ref = uint32_t;
using Size = uint32_t;

struct SectionEntry {
    char name[8];
    uint32_t flags;
    Ref offset;
    Size fileSize;
    Size memSize;
};

struct Header {
    char jump[10]; // jump to something in case ILDF is no recognized
    char magic[4]; // "ILDF"
    uint16_t numSections;
    SectionEntry sections[];
};

};
