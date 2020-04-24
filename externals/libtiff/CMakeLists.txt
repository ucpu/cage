cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "libtiff")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_TOOLS 1)
set(SKIP_TEST 1)
set(SKIP_DOC 1)
set(SKIP_CONTRIB 1)

option(jpeg "use libjpeg (required for JPEG compression)" ON)
option(old-jpeg "support for Old JPEG compression (read-only)" OFF)
option(jbig "use ISO JBIG compression (requires JBIT-KIT library)" OFF)
option(lzma "use liblzma (required for LZMA2 compression)" OFF)
option(webp "use libwebp (required for WEBP compression)" OFF)

add_subdirectory(libtiff)

target_include_directories(tiff INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libtiff/libtiff")
target_include_directories(tiff INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/libtiff/libtiff")

cage_gen_module(TIFF "$<TARGET_LINKER_FILE:tiff>;$<TARGET_LINKER_FILE:zlib>" $<TARGET_PROPERTY:tiff,INTERFACE_INCLUDE_DIRECTORIES>)
