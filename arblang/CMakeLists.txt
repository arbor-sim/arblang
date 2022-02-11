set(arblang-sources
    lexer.cpp
    parser.cpp
    raw_expressions.cpp
    resolved_expressions.cpp
    resolved_types.cpp
    type_expressions.cpp
    token.cpp
    unit_expressions.cpp
    normalizer.cpp
    canonicalize.cpp)

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