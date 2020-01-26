cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib-ng")
message(STATUS "----------------------------------------------------------------")

option(ZLIB_COMPAT "Compile with zlib compatible API" ON)
option(ZLIB_ENABLE_TESTS "Build test binaries" OFF)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_INSTALL_ALL ON)
unset(ENV{CI})

add_subdirectory(zlib-ng)

target_include_directories(zlib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng")
target_include_directories(zlib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng")

cage_gen_module(ZLIB $<TARGET_LINKER_FILE:zlib> "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng;${CMAKE_CURRENT_BINARY_DIR}/zlib-ng")
