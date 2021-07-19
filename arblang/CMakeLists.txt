set(arblang-sources
    lexer.cpp
    token.cpp)

add_library(arblang ${arblang-sources})

add_library(arblang-public-headers INTERFACE)
add_library(arblang-private-headers INTERFACE)

target_include_directories(arblang-public-headers INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_include_directories(arblang-private-headers INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_include_directories(arblang PUBLIC)
target_link_libraries(arblang PUBLIC arblang-public-headers)
target_link_libraries(arblang PRIVATE arblang-private-deps)

install(DIRECTORY include/arblang
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")