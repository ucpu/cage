cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libtiff")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

add_subdirectory(libtiff)

get_property(TIFF_INCLUDE_DIR TARGET tiff PROPERTY INCLUDE_DIRECTORIES)
set(TIFF_LIBRARY $<TARGET_FILE:tiff>)

set(filename ${CMAKE_BINARY_DIR}/cmake/FindTIFF.cmake)

file(WRITE ${filename} "set(TIFF_FOUND 1)\n")
file(APPEND ${filename} "set(TIFF_INCLUDE_DIR ${TIFF_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(TIFF_INCLUDE_DIRS ${TIFF_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(TIFF_LIBRARY ${TIFF_LIBRARY})\n")
file(APPEND ${filename} "set(TIFF_LIBRARIES ${TIFF_LIBRARY})\n")
