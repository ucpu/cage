cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "utfcpp")
message(STATUS "----------------------------------------------------------------")

add_library(utfcpp INTERFACE)
target_include_directories(utfcpp INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/utfcpp/source")

