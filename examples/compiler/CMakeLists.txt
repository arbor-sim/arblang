add_executable(compiler EXCLUDE_FROM_ALL compiler.cpp)
add_dependencies(examples compiler)

target_link_libraries(compiler PRIVATE arblang)
target_include_directories(compiler PUBLIC ../../ext/tinyopt/include)
