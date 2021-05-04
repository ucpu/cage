cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libpng")
message(STATUS "----------------------------------------------------------------")

option(PNG_SHARED "Build shared lib" OFF)
option(PNG_TESTS  "Build libpng tests" OFF)
set(SKIP_INSTALL_ALL ON)

# prevent using awk for configuration - it causes spurious rebuilds of the entire build
find_program(AWK NAMES gawk awk)
set(AWK "")

add_subdirectory(libpng)

target_include_directories(png_static INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libpng")
target_include_directories(png_static INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/libpng")

cage_gen_module(PNG $<TARGET_LINKER_FILE:png_static> $<TARGET_PROPERTY:png_static,INTERFACE_INCLUDE_DIRECTORIES>)
