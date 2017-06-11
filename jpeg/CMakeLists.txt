cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

add_subdirectory(jpeg)

get_property(JPEG_INCLUDE_DIR TARGET jpeg PROPERTY INCLUDE_DIRECTORIES)
set(JPEG_LIBRARY $<TARGET_FILE:jpeg>)

set(filename ${CMAKE_BINARY_DIR}/cmake/FindJPEG.cmake)

file(WRITE ${filename} "set(JPEG_FOUND 1)\n")
file(APPEND ${filename} "set(JPEG_INCLUDE_DIR ${JPEG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(JPEG_INCLUDE_DIRS ${JPEG_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(JPEG_LIBRARY ${JPEG_LIBRARY})\n")
file(APPEND ${filename} "set(JPEG_LIBRARIES ${JPEG_LIBRARY})\n")
