cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libtiff")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")

add_subdirectory(libtiff)

set(TIFF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libtiff/libtiff ${CMAKE_CURRENT_BINARY_DIR}/libtiff/libtiff)
set(TIFF_LIBRARY $<TARGET_LINKER_FILE:tiff> $<TARGET_LINKER_FILE:zlib>)
set(filename ${CMAKE_BINARY_DIR}/cmake/FindTIFF.cmake)
file(WRITE ${filename} "set(TIFF_FOUND 1)\n")
file(APPEND ${filename} "set(TIFF_INCLUDE_DIR ${TIFF_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(TIFF_INCLUDE_DIRS ${TIFF_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(TIFF_LIBRARY ${TIFF_LIBRARY})\n")
file(APPEND ${filename} "set(TIFF_LIBRARIES ${TIFF_LIBRARY})\n")
