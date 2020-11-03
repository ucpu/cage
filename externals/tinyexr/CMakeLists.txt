cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "tinyexr")
message(STATUS "----------------------------------------------------------------")

add_library(tinyexr INTERFACE)
target_sources(tinyexr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tinyexr/tinyexr.h)
target_include_directories(tinyexr INTERFACE tinyexr)

