cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "tiff")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(ZSTD_HAVE_DECOMPRESS_STREAM ON) # avoid check that would have failed - we know we have sufficient version of zstd
set(HAVE_JPEGTURBO_DUAL_MODE_8_12 OFF) # avoid check that would have failed

set(tiff-tools OFF CACHE INTERNAL "build TIFF tools")
set(tiff-tests OFF CACHE INTERNAL "build TIFF tests")
set(tiff-contrib OFF CACHE INTERNAL "build TIFF contrib")
set(tiff-docs OFF CACHE INTERNAL "build TIFF documentation")
set(tiff-deprecated OFF CACHE INTERNAL "build TIFF deprecated features")
set(tiff-install OFF CACHE INTERNAL "install TIFF targets")
set(webp OFF CACHE INTERNAL "use libwebp (required for WEBP compression)") # webp causes linker errors - explicitly disabling

add_subdirectory(tiff)

set_source_files_properties("tiff/libtiff/tif_win32_versioninfo.rc" TARGET_DIRECTORY tiff PROPERTIES HEADER_FILE_ONLY ON) # exclude this file from compilation - it fails with clang-cl

target_include_directories(tiff INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libtiff/libtiff")
target_include_directories(tiff INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/libtiff/libtiff")

cage_gen_module(TIFF "$<TARGET_LINKER_FILE:tiff>;$<TARGET_LINKER_FILE:zlib>" $<TARGET_PROPERTY:tiff,INTERFACE_INCLUDE_DIRECTORIES>)
