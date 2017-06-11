cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libpng")
message(STATUS "----------------------------------------------------------------")

option(PNG_SHARED "Build shared lib" OFF)
option(PNG_TESTS  "Build libpng tests" OFF)

add_subdirectory(libpng)

get_property(PNG_INCLUDE_DIR TARGET png_static PROPERTY INCLUDE_DIRECTORIES)
set(PNG_LIBRARY $<TARGET_FILE:png_static>)

set(filename ${CMAKE_BINARY_DIR}/cmake/FindPNG.cmake)

file(WRITE ${filename} "set(PNG_FOUND 1)\n")
file(APPEND ${filename} "set(PNG_INCLUDE_DIR ${PNG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(PNG_LIBRARY ${PNG_LIBRARY})\n")
file(APPEND ${filename} "set(PNG_LIBRARIES ${PNG_LIBRARY})\n")

