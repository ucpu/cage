cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "xatlas")
message(STATUS "----------------------------------------------------------------")

project(xatlas CXX)

add_library(xatlas STATIC "xatlas/source/xatlas/xatlas.h" "xatlas/source/xatlas/xatlas.cpp")

target_include_directories(xatlas PUBLIC "xatlas/source/xatlas")
target_compile_definitions(xatlas PRIVATE "XA_MULTITHREADED=0")

if(NOT MSVC)
	target_link_libraries(xatlas PUBLIC pthread)
endif()
