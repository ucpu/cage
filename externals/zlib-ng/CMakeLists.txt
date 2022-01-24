cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib-ng")
message(STATUS "----------------------------------------------------------------")

option(ZLIB_COMPAT "Compile with zlib compatible API" ON)
option(ZLIB_ENABLE_TESTS "Build test binaries" OFF)
set(WITH_SANITIZER OFF CACHE INTERNAL "Enable sanitizer support")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_INSTALL_ALL ON)
unset(ENV{CI})

add_subdirectory(zlib-ng)

target_include_directories(zlib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng")
target_include_directories(zlib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng")

# pnglib requires exact paths otherwise its generated files will fail to include zlib
cage_gen_module(ZLIB $<TARGET_LINKER_FILE:zlib> "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng;${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng")

add_library(ZLIB::ZLIB ALIAS zlib)
