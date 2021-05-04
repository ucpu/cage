cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "plf")
message(STATUS "----------------------------------------------------------------")

add_library(plf STATIC plf_colony/plf_colony.h plf_list/plf_list.h plf_stack/plf_stack.h dummy.cpp)
target_include_directories(robin-hood-hashing PUBLIC plf_colony plf_list plf_stack)

