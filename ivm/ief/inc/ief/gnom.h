#pragma once
#include "./core.h"
#include <cstdint>

namespace ief::gnom {

struct Variable {
    Ref nameString;
    Ref typeNameString;
    int32_t offset;
    uint32_t size; // 0 for optimized out variable
    uint16_t section; // 0 for variable on stack, relative to frame
};

struct Strucure {
    
};

struct Function {
    
};

};
