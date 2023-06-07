cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "tinyexr")
message(STATUS "----------------------------------------------------------------")

set(TINYEXR_BUILD_SAMPLE OFF CACHE INTERNAL "Build a sample")
set(TINYEXR_USE_MINIZ OFF CACHE INTERNAL "Use miniz")

add_library(tinyexr STATIC tinyexr/tinyexr.h tinyexr.cpp)
target_include_directories(tinyexr PUBLIC tinyexr)
target_link_libraries(tinyexr PRIVATE zlib)

