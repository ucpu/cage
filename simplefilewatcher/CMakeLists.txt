cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "simplefilewatcher")
message(STATUS "----------------------------------------------------------------")

file(GLOB sources simplefilewatcher/source/*)
add_library(simplefilewatcher STATIC ${sources})
target_include_directories(simplefilewatcher PUBLIC simplefilewatcher/include)
