cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "devil")
message(STATUS "----------------------------------------------------------------")

add_subdirectory(devil)

get_property(IL_INCLUDE_DIR TARGET devil PROPERTY INCLUDE_DIRECTORIES)
set(IL_LIBRARY $<TARGET_FILE:devil>)

set(filename ${CMAKE_BINARY_DIR}/cmake/FindIL.cmake)

file(WRITE ${filename} "set(IL_FOUND 1)\n")
file(APPEND ${filename} "set(IL_INCLUDE_DIR ${IL_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(IL_LIBRARY ${IL_LIBRARY})\n")

target_include_directories(devil INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/include)
