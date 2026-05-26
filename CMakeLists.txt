cmake_minimum_required(VERSION 3.16)
project(2dmultiplayergame C)

find_package(raylib REQUIRED)

add_executable(main src/main.c)
target_link_libraries(main PRIVATE raylib)

set_target_properties(main PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
)
