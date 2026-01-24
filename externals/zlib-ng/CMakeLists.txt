message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib-ng")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

set(CMAKE_C_STANDARD 11)

set(ZLIB_COMPAT ON CACHE INTERNAL "Compile with zlib compatible API")
set(BUILD_TESTING OFF CACHE INTERNAL "Build test binaries")
set(WITH_SANITIZER OFF CACHE INTERNAL "Enable sanitizer support")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_INSTALL_ALL ON CACHE INTERNAL "SKIP_INSTALL_ALL")
unset(ENV{CI})

add_subdirectory(zlib-ng)

target_include_directories(zlib-ng PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng")
target_include_directories(zlib-ng INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng")

# pnglib requires exact paths otherwise its generated files will fail to include zlib
cage_gen_module(ZLIB $<TARGET_LINKER_FILE:zlib-ng> "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng;${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng")

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng/zlib.h.in _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([0-9]+.[0-9]+.[0-9]+).*\".*" "\\1" ZLIB_HEADER_VERSION ${_zlib_h_contents})
message(STATUS "zlib version: ${ZLIB_HEADER_VERSION}")
set(filename "${CAGE_EXTERNALS_MODULE_PATH}/FindZLIB.cmake")
file(APPEND ${filename} "set(ZLIB_VERSION ${ZLIB_HEADER_VERSION})\n")

add_library(ZLIB::ZLIB ALIAS zlib-ng)
