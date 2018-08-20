cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib")
message(STATUS "----------------------------------------------------------------")

set(SKIP_EXAMPLES 1)
set(SKIP_INSTALL_ALL 1)

add_subdirectory(zlib)

target_include_directories(zlib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/zlib")
target_include_directories(zlib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zlib")

cage_gen_module(ZLIB $<TARGET_LINKER_FILE:zlib> "${CMAKE_CURRENT_SOURCE_DIR}/zlib;${CMAKE_CURRENT_BINARY_DIR}/zlib")
