cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libsoundio")
message(STATUS "----------------------------------------------------------------")

option(BUILD_STATIC_LIBS "Build static libraries" OFF)
option(BUILD_DYNAMIC_LIBS "Build dynamic libraries" ON)
option(BUILD_EXAMPLE_PROGRAMS "Build example programs" OFF)
option(BUILD_TESTS "Build tests" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

add_subdirectory(libsoundio)

if(MSVC)
    target_compile_options(libsoundio_shared PRIVATE "/wd4577")
    target_compile_options(libsoundio_shared PRIVATE "/W0")
endif(MSVC)

target_include_directories(libsoundio_shared INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libsoundio)

