cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "tinyexr")
message(STATUS "----------------------------------------------------------------")

add_library(tinyexr STATIC tinyexr/tinyexr.h tinyexr.cpp)
target_include_directories(tinyexr PUBLIC tinyexr)
target_link_libraries(tinyexr PRIVATE zlib)

