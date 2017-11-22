cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "devil")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "BUILD_SHARED_LIBS")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/cmake/Modules")

add_subdirectory(devil/DevIL/src-IL)
add_subdirectory(devil/DevIL/src-ILU)

target_include_directories(IL INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/include)

set(filename ${CMAKE_BINARY_DIR}/cmake/FindIL.cmake)
file(WRITE ${filename} "set(IL_FOUND 1)\n")
file(APPEND ${filename} "set(IL_LIBRARY IL)\n")

