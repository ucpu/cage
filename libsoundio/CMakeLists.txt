cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libsoundio")
message(STATUS "----------------------------------------------------------------")

set(BUILD_STATIC_LIBS ON CACHE STRING "Build static libraries" FORCE)
set(BUILD_DYNAMIC_LIBS OFF CACHE STRING "Build dynamic libraries" FORCE)
option(BUILD_EXAMPLE_PROGRAMS "Build example programs" OFF)
option(BUILD_TESTS "Build tests" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory(libsoundio)

if(MSVC)
    target_compile_options(libsoundio_static PRIVATE "/wd4577")
    target_compile_options(libsoundio_static PRIVATE "/W0")
    target_compile_definitions(libsoundio_static PUBLIC "SOUNDIO_STATIC_LIBRARY")
endif(MSVC)

target_include_directories(libsoundio_static INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libsoundio")

