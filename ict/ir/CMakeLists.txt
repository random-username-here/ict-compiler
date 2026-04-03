project(ict-ir-frontend)

add_library(ict-ir-frontend MODULE)
set_target_properties(ict-ir-frontend PROPERTIES OUTPUT_NAME "ict.ir.frontend" SUFFIX ".mod")
target_include_directories(ict-ir-frontend PRIVATE src/)
target_link_libraries(ict-ir-frontend PUBLIC ict-lib)
target_sources(ict-ir-frontend PRIVATE mod.cpp)
