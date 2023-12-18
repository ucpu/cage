cmake_minimum_required(VERSION 3.20)

message(STATUS "----------------------------------------------------------------")
message(STATUS "quickhull")
message(STATUS "----------------------------------------------------------------")

file(GLOB SOURCES quickhull/*.cpp quickhull/*.hpp quickhull/Structs/*)

add_library(quickhull STATIC ${SOURCES})

target_include_directories(quickhull INTERFACE .)
