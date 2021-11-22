cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "glad")
message(STATUS "----------------------------------------------------------------")

add_library(glad STATIC glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)
if(MSVC)
	target_compile_definitions(glad PRIVATE "GLAPI=__declspec(dllexport)")
else()
	target_compile_definitions(glad PRIVATE "GLAPI=__attribute__((visibility(\"default\")))")
endif()
