cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "lodepng")
message(STATUS "----------------------------------------------------------------")

project(lodepng VERSION 0 LANGUAGES CXX)
add_library(lodepng STATIC lodepng/lodepng.cpp)
target_include_directories(lodepng INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lodepng)
