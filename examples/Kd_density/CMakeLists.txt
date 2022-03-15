add_executable(kd_density EXCLUDE_FROM_ALL Kd_density.cpp)
add_dependencies(examples kd_density)

target_link_libraries(kd_density PRIVATE arblang)
target_include_directories(kd_density PUBLIC ../../ext/tinyopt/include)
