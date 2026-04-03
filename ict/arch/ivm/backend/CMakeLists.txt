project(ict-ivm)

add_library(ict-ar-ivm INTERFACE)
target_include_directories(ict-ar-ivm INTERFACE inc/)

add_library(ict-ivm MODULE)
set_target_properties(ict-ivm PROPERTIES OUTPUT_NAME "ict.ivm.backend" SUFFIX ".mod")
target_include_directories(ict-ivm PRIVATE src/)
target_link_libraries(ict-ivm PUBLIC ict-lib ict-ar-ivm)
target_sources(ict-ivm PRIVATE src/mod.cpp)

