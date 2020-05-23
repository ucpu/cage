cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "xatlas")
message(STATUS "----------------------------------------------------------------")

project(xatlas CXX)

add_library(xatlas STATIC "xatlas/xatlas.h" "xatlas/xatlas.cpp")

target_include_directories(xatlas PUBLIC xatlas)

if(NOT MSVC)
	target_link_libraries(xatlas PUBLIC pthread)
endif()
