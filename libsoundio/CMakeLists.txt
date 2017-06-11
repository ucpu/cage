cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libsoundio")
message(STATUS "----------------------------------------------------------------")

option(BUILD_STATIC_LIBS "Build static libraries" OFF)
option(BUILD_DYNAMIC_LIBS "Build dynamic libraries" ON)
option(BUILD_EXAMPLE_PROGRAMS "Build example programs" OFF)
option(BUILD_TESTS "Build tests" OFF)

set(CMAKE_BUILD_TYPE Release)

add_subdirectory(libsoundio)

target_include_directories(libsoundio_shared INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libsoundio)

add_library(libsoundio ALIAS libsoundio_shared)
