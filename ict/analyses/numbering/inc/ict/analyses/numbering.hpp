#pragma once
#include "ict/ir.hpp"
#include <cstdint>
#include <unordered_map>
namespace ict::an {

class NumberingAnalyzer;

class Numbering : public ict::Analysis {
    friend class NumberingAnalyzer;
    std::unordered_map<const void*, size_t> m_num;
public:
    uintptr_t get(const void* item) const { return m_num.count(item) ? m_num.at(item) : (uintptr_t) item; }
};

};
