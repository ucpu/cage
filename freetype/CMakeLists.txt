cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "freetype")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(SKIP_INSTALL_ALL ON)

add_subdirectory(freetype)

set(FREETYPE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/freetype/include ${CMAKE_CURRENT_BINARY_DIR}/freetype/include)
set(FREETYPE_LIBRARY $<TARGET_LINKER_FILE:freetype>)
set(filename ${CMAKE_BINARY_DIR}/cmake/FindFreetype.cmake)
file(WRITE ${filename} "set(FREETYPE_FOUND 1)\n")
file(APPEND ${filename} "set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(FREETYPE_LIBRARY ${FREETYPE_LIBRARY})\n")
file(APPEND ${filename} "set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})\n")

target_include_directories(freetype INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/freetype/include)

