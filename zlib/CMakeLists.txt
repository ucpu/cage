cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib")
message(STATUS "----------------------------------------------------------------")

add_subdirectory(zlib)

add_library(zlibstatic ALIAS z)

get_property(ZLIB_INCLUDE_DIR TARGET zlibstatic PROPERTY INCLUDE_DIRECTORIES)
set(ZLIB_LIBRARY $<TARGET_FILE:zlibstatic>)

set(filename ${CMAKE_BINARY_DIR}/cmake/FindZLIB.cmake)

file(WRITE ${filename} "set(ZLIB_FOUND 1)\n")
file(APPEND ${filename} "set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(ZLIB_LIBRARY ${ZLIB_LIBRARY})\n")
file(APPEND ${filename} "set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})\n")

target_include_directories(z INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
