cmake_minimum_required(VERSION 3.10)
project(final)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
add_compile_options(-g)

add_subdirectory(misclib)
add_subdirectory(modlib)

add_subdirectory(ict/core)
add_subdirectory(ict/ir)
#add_subdirectory(ict/analyses/below)
#add_subdirectory(ict/analyses/crossblock)
add_subdirectory(ict/arch/ivm/backend)
add_subdirectory(ict/arch/ivm/hl2ll)
add_subdirectory(ict/arch/ivm/basic-stackplan)

add_subdirectory(scl)


