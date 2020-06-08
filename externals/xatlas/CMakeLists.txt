cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "xatlas")
message(STATUS "----------------------------------------------------------------")

project(xatlas CXX)

add_library(xatlas STATIC "xatlas/xatlas.h" "xatlas/xatlas.cpp")

target_include_directories(xatlas PUBLIC xatlas)
target_compile_definitions(xatlas PRIVATE "XA_MULTITHREADED=0")

if(NOT MSVC)
	target_link_libraries(xatlas PUBLIC pthread)
endif()
