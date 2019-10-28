cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib")
message(STATUS "----------------------------------------------------------------")

set(SKIP_EXAMPLES 1)
set(SKIP_INSTALL_ALL 1)
set(ZLIB_STATIC_ONLY 1 CACHE INTERNAL "ZLIB_STATIC_ONLY")

add_subdirectory(zlib)

target_include_directories(zlibstatic PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/zlib")
target_include_directories(zlibstatic INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zlib")

cage_gen_module(ZLIB $<TARGET_LINKER_FILE:zlib> "${CMAKE_CURRENT_SOURCE_DIR}/zlib;${CMAKE_CURRENT_BINARY_DIR}/zlib")
