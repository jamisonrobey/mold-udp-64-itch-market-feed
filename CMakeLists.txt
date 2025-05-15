cmake_minimum_required(VERSION 3.16)

project(mold_udp_itch_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file (GLOB_RECURSE SOURCES CMAKE_CONFIGURE_DEPENDS "src/*.cpp")

add_executable(server ${SOURCES})

add_subdirectory(external/jam_utils)
target_link_libraries(server PRIVATE jam_utils)
