set(arblang-sources
    optimizer/constant_fold.cpp
    optimizer/copy_propagate.cpp
    optimizer/cse.cpp
    optimizer/eliminate_dead_code.cpp
    optimizer/inline_func.cpp
    parser/lexer.cpp
    parser/parser.cpp
    parser/parsed_expressions.cpp
    parser/parsed_types.cpp
    parser/parsed_units.cpp
    parser/token.cpp
    parser/normalizer.cpp
    pre_printer/check_mechanism.cpp
    pre_printer/get_read_arguments.cpp
    pre_printer/printable_mechanism.cpp
    pre_printer/simplify.cpp
    resolver/resolve.cpp
    resolver/resolved_expressions.cpp
    resolver/resolved_types.cpp
    resolver/canonicalize.cpp
    resolver/single_assign.cpp
    solver/solve.cpp
    solver/solve_ode.cpp
    solver/symbolic_diff.cpp
    util/pretty_printer.cpp
    util/rexp_helpers.cpp
)

add_library(arblang ${arblang-sources})

add_library(arblang-public-headers INTERFACE)
add_library(arblang-private-headers INTERFACE)

target_include_directories(arblang PRIVATE ../ext/fmt/include)

target_include_directories(arblang-public-headers INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_include_directories(arblang-private-headers INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_include_directories(arblang PUBLIC)
target_link_libraries(arblang PUBLIC arblang-public-headers)
target_link_libraries(arblang PRIVATE arblang-private-deps)

target_compile_definitions(arblang PRIVATE FMT_HEADER_ONLY)

#find_package(fmt)
#target_link_libraries(arblang PRIVATE fmt::fmt-header-only)

install(DIRECTORY include/arblang
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")