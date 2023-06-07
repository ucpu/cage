cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "dualmc")
message(STATUS "----------------------------------------------------------------")

add_library(dualmc STATIC "dualmc/include/dualmc.h" "dummy.cpp")

target_include_directories(dualmc PUBLIC dualmc/include)
