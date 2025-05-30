message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib-ng")
message(STATUS "----------------------------------------------------------------")

set(CMAKE_C_STANDARD 11)

set(ZLIB_COMPAT ON CACHE INTERNAL "Compile with zlib compatible API")
set(ZLIB_ENABLE_TESTS OFF CACHE INTERNAL "Build test binaries")
set(WITH_SANITIZER OFF CACHE INTERNAL "Enable sanitizer support")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_INSTALL_ALL ON CACHE INTERNAL "SKIP_INSTALL_ALL")
unset(ENV{CI})

add_subdirectory(zlib-ng)

target_include_directories(zlib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng")
target_include_directories(zlib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng")

# pnglib requires exact paths otherwise its generated files will fail to include zlib
cage_gen_module(ZLIB $<TARGET_LINKER_FILE:zlib> "${CMAKE_CURRENT_BINARY_DIR}/zlib-ng;${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng")

add_library(ZLIB::ZLIB ALIAS zlib)
