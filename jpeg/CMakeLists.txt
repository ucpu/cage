cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")

add_subdirectory(jpeg)

set(JPEG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jpeg ${CMAKE_CURRENT_BINARY_DIR}/jpeg)
set(JPEG_LIBRARY $<TARGET_LINKER_FILE:jpeg>)
set(filename ${CMAKE_BINARY_DIR}/cmake/FindJPEG.cmake)
file(WRITE ${filename} "set(JPEG_FOUND 1)\n")
file(APPEND ${filename} "set(JPEG_INCLUDE_DIR ${JPEG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(JPEG_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(JPEG_LIBRARY ${JPEG_LIBRARY})\n")
file(APPEND ${filename} "set(JPEG_LIBRARIES ${JPEG_LIBRARY})\n")

