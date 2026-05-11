project(ict-ivm-stackplan)

add_library(ict-ivm-stackplan MODULE)
set_target_properties(ict-ivm-stackplan PROPERTIES OUTPUT_NAME "ict.ivm.stackplan" SUFFIX ".mod" PREFIX "")
target_link_libraries(ict-ivm-stackplan PUBLIC ict-lib ict-ar-ivm ict-an-numbering)
target_sources(ict-ivm-stackplan PRIVATE mod.cpp)
