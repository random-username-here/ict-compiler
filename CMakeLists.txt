cmake_minimum_required(VERSION 3.10)
project(final)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
add_compile_options(-g)

add_subdirectory(misclib)
add_subdirectory(modlib)

# IR base
add_subdirectory(ict/core)

# Analyses
add_subdirectory(ict/analyses/numbering)

# Passes
add_subdirectory(ict/passes/dehumanify)

# IVM Backend
add_subdirectory(ict/arch/ivm/backend)
add_subdirectory(ict/arch/ivm/hl2ll)
add_subdirectory(ict/arch/ivm/basic-stackplan)

# Optimization passes for IVM
add_subdirectory(ict/arch/ivm/passes/reorder)

# IR frontend
add_subdirectory(ict/ir)

# Language frontend
add_subdirectory(scl)


