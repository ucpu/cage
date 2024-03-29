cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "zstd")
message(STATUS "----------------------------------------------------------------")

set(ZSTD_BUILD_STATIC ON CACHE INTERNAL "ZSTD_BUILD_STATIC")
set(ZSTD_BUILD_SHARED OFF CACHE INTERNAL "ZSTD_BUILD_SHARED")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/zstd/build/cmake/CMakeModules")
set(ZSTD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zstd")
set(LIBRARY_DIR ${ZSTD_SOURCE_DIR}/lib)
include(AddZstdCompilationFlags)
ADD_ZSTD_COMPILATION_FLAGS()
add_definitions(-DXXH_NAMESPACE=ZSTD_)
include(GNUInstallDirs)

add_subdirectory(zstd/build/cmake/lib)

target_include_directories(libzstd_static PUBLIC zstd/lib zstd/lib/common)

cage_gen_module(Zstd $<TARGET_LINKER_FILE:libzstd_static> $<TARGET_PROPERTY:libzstd_static,INTERFACE_INCLUDE_DIRECTORIES>)

add_library(Zstd::Zstd ALIAS libzstd_static)
add_library(ZSTD::ZSTD ALIAS libzstd_static)
