cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "devil")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "BUILD_SHARED_LIBS")

add_subdirectory(devil/DevIL/src-IL)
add_subdirectory(devil/DevIL/src-ILU)

target_include_directories(IL PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/devil/DevIL/src-IL/include)
target_include_directories(IL INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/include)

set(IL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/include)
set(IL_LIBRARY $<TARGET_LINKER_FILE:IL>)
set(filename ${CMAKE_BINARY_DIR}/cmake/FindIL.cmake)
file(WRITE ${filename} "set(IL_FOUND 1)\n")
file(APPEND ${filename} "set(IL_INCLUDE_DIR ${IL_INCLUDE_DIR})\n")
file(APPEND ${filename} "set(IL_LIBRARY ${IL_LIBRARY})\n")

