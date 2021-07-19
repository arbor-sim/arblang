cmake_minimum_required(VERSION 3.9)
project(arblang-compiler LANGUAGES CXX)
ENABLE_LANGUAGE(C)

cmake_policy(SET CMP0074 NEW)
set (CMAKE_CXX_STANDARD 14)

set(CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(arblang-private-deps INTERFACE)
target_link_libraries(arblang-private-deps INTERFACE)
install(TARGETS arblang-private-deps EXPORT arblang-targets)

add_library(arblang-public-deps INTERFACE)
install(TARGETS arblang-public-deps EXPORT arblang-targets)

add_subdirectory(arblang)

