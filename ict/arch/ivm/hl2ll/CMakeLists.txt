project(ict-ivm-hl2ll)

add_library(ict-ivm-hl2ll MODULE)
set_target_properties(ict-ivm-hl2ll PROPERTIES OUTPUT_NAME "ict.ivm.hl2ll" SUFFIX ".mod")
target_link_libraries(ict-ivm-hl2ll PUBLIC ict-lib ict-ar-ivm)
target_sources(ict-ivm-hl2ll PRIVATE mod.cpp)
