project(ict)

add_library(ict-lib SHARED)
target_include_directories(ict-lib PUBLIC inc/)
target_link_libraries(ict-lib PUBLIC misclib modlib)
target_sources(ict-lib PRIVATE src/ir.cpp src/mod.cpp)

add_executable(ict-example-ivm EXCLUDE_FROM_ALL)
target_link_libraries(ict-example-ivm PRIVATE ict-lib)
target_sources(ict-example-ivm PRIVATE demos/ivm.cpp)

add_executable(ict)
target_link_libraries(ict PUBLIC ict-lib ict-an-below ict-an-crossblock)
target_sources(ict PRIVATE src/main.cpp)
