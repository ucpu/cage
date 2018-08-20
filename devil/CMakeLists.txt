cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "devil")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "BUILD_SHARED_LIBS")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/cmake/Modules")
set(IL_NO_JP2 ON CACHE INTERNAL "no jasper lib") # japser has include error on mac

add_subdirectory(devil/DevIL/src-IL)
add_subdirectory(devil/DevIL/src-ILU)

target_include_directories(IL INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/devil/DevIL/include")

cage_gen_module(IL IL "")
