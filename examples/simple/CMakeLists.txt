add_executable(simple EXCLUDE_FROM_ALL simple.cpp)
add_dependencies(examples simple)

target_link_libraries(simple PRIVATE arblang)
