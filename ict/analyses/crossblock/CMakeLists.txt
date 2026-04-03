project(ict-an-crossblock)

add_library(ict-an-crossblock STATIC)
target_link_libraries(ict-an-crossblock PUBLIC ict-lib)
target_include_directories(ict-an-crossblock PUBLIC inc/)
target_sources(ict-an-crossblock PRIVATE ifc.cpp)

add_library(ict-an-crossblock-mod MODULE)
set_target_properties(ict-an-crossblock-mod PROPERTIES OUTPUT_NAME "ict.an.crossblock" SUFFIX ".mod")
target_link_libraries(ict-an-crossblock-mod PUBLIC ict-an-crossblock ict-an-below)
target_sources(ict-an-crossblock-mod PRIVATE mod.cpp)

