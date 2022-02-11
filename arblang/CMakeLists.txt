set(arblang-sources
    parser/lexer.cpp
    parser/parser.cpp
    parser/parsed_expressions.cpp
    parser/parsed_types.cpp
    parser/token.cpp
    parser/normalizer.cpp
    parser/unit_expressions.cpp
    resolver/resolved_expressions.cpp
    resolver/resolved_types.cpp
    resolver/canonicalize.cpp)

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