# Build mechanisms used solely in unit tests.
set(unit_sources
    test_lexer.cpp

    # unit test driver
    test.cpp
    )

add_executable(unit ${unit_sources})
add_dependencies(tests unit)

target_compile_definitions(unit PRIVATE "-DDATADIR=\"${CMAKE_CURRENT_SOURCE_DIR}/input\"")
target_include_directories(unit PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(unit PRIVATE gtest arblang)
