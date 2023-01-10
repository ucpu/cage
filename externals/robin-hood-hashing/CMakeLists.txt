cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "robin-hood-hashing")
message(STATUS "----------------------------------------------------------------")

add_library(robin-hood-hashing INTERFACE)
target_include_directories(robin-hood-hashing INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/robin-hood-hashing/src/include>)
