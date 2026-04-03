project(misclib)

add_library(misclib SHARED)
target_include_directories(misclib PUBLIC inc/)
target_sources(misclib PRIVATE src/shortvec.cpp src/parse.cpp src/misc.cpp)

add_executable(misclib-example-shortvec EXCLUDE_FROM_ALL)
target_link_libraries(misclib-example-shortvec PRIVATE misclib)
target_sources(misclib-example-shortvec PRIVATE demos/shortvec.cpp)

add_executable(misclib-example-parse EXCLUDE_FROM_ALL)
target_link_libraries(misclib-example-parse PRIVATE misclib)
target_sources(misclib-example-parse PRIVATE demos/parse.cpp)

add_executable(misclib-example-dstream EXCLUDE_FROM_ALL)
target_link_libraries(misclib-example-dstream PRIVATE misclib)
target_sources(misclib-example-dstream PRIVATE demos/dstream.cpp)

add_executable(misclib-example-ast EXCLUDE_FROM_ALL)
target_link_libraries(misclib-example-ast PRIVATE misclib)
target_sources(misclib-example-ast PRIVATE demos/ast.cpp)
