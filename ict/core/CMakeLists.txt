project(ict)

add_library(ict-lib SHARED)
target_include_directories(ict-lib PUBLIC inc/)
target_link_libraries(ict-lib PUBLIC misclib modlib)
target_sources(ict-lib PRIVATE src/ir.cpp src/mod.cpp src/ir.verify.cpp)

add_executable(ict-example-playground EXCLUDE_FROM_ALL)
target_link_libraries(ict-example-playground PRIVATE ict-lib)
target_sources(ict-example-playground PRIVATE demos/playground.cpp)

add_executable(ict)
target_link_libraries(ict PUBLIC ict-lib)
target_sources(ict PRIVATE src/main.cpp)
