cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "plf")
message(STATUS "----------------------------------------------------------------")

add_library(plf INTERFACE)
target_include_directories(robin-hood-hashing INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plf_colony> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plf_list> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/plf_stack>)
