cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "glad")
message(STATUS "----------------------------------------------------------------")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_library(glad STATIC glad/src/glad.c)
target_include_directories(glad PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>)
if(MSVC)
	target_compile_definitions(glad PRIVATE "GLAPI=__declspec(dllexport)")
else()
	target_compile_definitions(glad PRIVATE "GLAPI=__attribute__((visibility(\"default\")))")
endif()
