project(scl-lang)

add_library(scl-lang MODULE 
    src/ast/expr.cpp src/ast/block.cpp
    src/parser/expr.cpp src/parser/block.cpp
    src/pass/scopes.cpp
    src/irgen.cpp src/mod.cpp
)

set_target_properties(scl-lang PROPERTIES OUTPUT_NAME "ict.ir.frontend" SUFFIX ".mod" PREFIX "")
target_include_directories(scl-lang PRIVATE inc/)
target_link_libraries(scl-lang PUBLIC ict-lib)
target_sources(scl-lang PRIVATE src/mod.cpp)
