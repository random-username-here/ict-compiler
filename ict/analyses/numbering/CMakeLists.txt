project(ict-an-numbering)

add_library(ict-an-numbering INTERFACE)
target_include_directories(ict-an-numbering INTERFACE inc/)
target_link_libraries(ict-an-numbering INTERFACE ict-lib)

add_library(ict-an-numbering-impl MODULE mod.cpp)
set_target_properties(ict-an-numbering-impl PROPERTIES OUTPUT_NAME "ict.an.numbering" SUFFIX ".mod" PREFIX "")
target_link_libraries(ict-an-numbering-impl PUBLIC ict-an-numbering)
