project(ict-dehumanify)

add_library(ict-dehumanify MODULE)
set_target_properties(ict-dehumanify PROPERTIES OUTPUT_NAME "ict.dehumanify" SUFFIX ".mod" PREFIX "")
target_link_libraries(ict-dehumanify PUBLIC ict-lib)
target_sources(ict-dehumanify PRIVATE mod.cpp)
