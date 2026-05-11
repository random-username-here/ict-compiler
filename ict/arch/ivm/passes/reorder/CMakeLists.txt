project(ict-ivm-reorder)

add_library(ict-ivm-reorder MODULE)
set_target_properties(ict-ivm-reorder PROPERTIES OUTPUT_NAME "ict.ivm.reorder" SUFFIX ".mod" PREFIX "")
target_link_libraries(ict-ivm-reorder PUBLIC ict-lib ict-ar-ivm)
target_sources(ict-ivm-reorder PRIVATE mod.cpp)
