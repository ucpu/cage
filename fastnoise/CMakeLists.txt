cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "fastnoise")
message(STATUS "----------------------------------------------------------------")

add_library(fastnoise STATIC fastnoise/FastNoise.h fastnoise/FastNoise.cpp)
target_include_directories(fastnoise PUBLIC .)

