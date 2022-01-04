cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "tinyexr")
message(STATUS "----------------------------------------------------------------")

option(TINYEXR_BUILD_SAMPLE "Build a sample" OFF)
option(TINYEXR_USE_MINIZ "Use miniz" OFF)

add_library(tinyexr STATIC tinyexr/tinyexr.h tinyexr.cpp)
target_include_directories(tinyexr PUBLIC tinyexr)
target_link_libraries(tinyexr PRIVATE zlib)

