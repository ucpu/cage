cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "png")
message(STATUS "----------------------------------------------------------------")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

option(PNG_SHARED "Build shared lib" OFF)
option(PNG_TESTS  "Build libpng tests" OFF)
set(SKIP_INSTALL_ALL ON)

# prevent using awk for configuration - it causes spurious rebuilds of the entire build
find_program(AWK NAMES gawk awk)
set(AWK "")

add_subdirectory(png)

target_include_directories(png_static INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/png")
target_include_directories(png_static INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/png")

cage_gen_module(PNG $<TARGET_LINKER_FILE:png_static> $<TARGET_PROPERTY:png_static,INTERFACE_INCLUDE_DIRECTORIES>)
