cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "robin-hood-hashing")
message(STATUS "----------------------------------------------------------------")

add_library(robin-hood-hashing STATIC robin-hood-hashing/src/include/robin_hood.h dummy.cpp)
target_include_directories(robin-hood-hashing PUBLIC robin-hood-hashing/src/include)

