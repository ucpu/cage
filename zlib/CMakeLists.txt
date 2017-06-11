cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "zlib")
message(STATUS "----------------------------------------------------------------")

set(SKIP_EXAMPLES 1)
set(SKIP_INSTALL_ALL 1)

add_subdirectory(zlib)

set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zlib ${CMAKE_CURRENT_BINARY_DIR}/zlib)
set(ZLIB_LIBRARY $<TARGET_LINKER_FILE:zlib>)
set(filename ${CMAKE_BINARY_DIR}/cmake/FindZLIB.cmake)
file(WRITE ${filename} "set(ZLIB_FOUND 1)\n")
file(APPEND ${filename} "set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(ZLIB_LIBRARY ${ZLIB_LIBRARY})\n")
file(APPEND ${filename} "set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})\n")

target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/zlib)
target_include_directories(zlib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
