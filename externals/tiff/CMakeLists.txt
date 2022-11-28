cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "tiff")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(ZSTD_HAVE_DECOMPRESS_STREAM TRUE) # avoid check that would have failed - we know we have sufficient version of zstd
set(HAVE_JPEGTURBO_DUAL_MODE_8_12 FALSE) # avoid check that would have failed

option(tiff-tools "build TIFF tools" OFF)
option(tiff-tests "build TIFF tests" OFF)
option(tiff-contrib "build TIFF contrib" OFF)
option(tiff-docs "build TIFF documentation" OFF)
option(tiff-deprecated "build TIFF deprecated features" OFF)
option(tiff-install "install TIFF targets" OFF)

option(webp "use libwebp (required for WEBP compression)" OFF) # webp causes linker errors - explicitly disabling

add_subdirectory(tiff)

target_include_directories(tiff INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libtiff/libtiff")
target_include_directories(tiff INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/libtiff/libtiff")

cage_gen_module(TIFF "$<TARGET_LINKER_FILE:tiff>;$<TARGET_LINKER_FILE:zlib>" $<TARGET_PROPERTY:tiff,INTERFACE_INCLUDE_DIRECTORIES>)
