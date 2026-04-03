project(ict-an-below)

add_library(ict-an-below STATIC)
target_link_libraries(ict-an-below PUBLIC ict-lib)
target_include_directories(ict-an-below PUBLIC inc/)
target_sources(ict-an-below PRIVATE ifc.cpp)

add_library(ict-an-below-mod MODULE)
set_target_properties(ict-an-below-mod PROPERTIES OUTPUT_NAME "ict.an.below" SUFFIX ".mod")
target_link_libraries(ict-an-below-mod PUBLIC ict-an-below)
target_sources(ict-an-below-mod PRIVATE mod.cpp)

