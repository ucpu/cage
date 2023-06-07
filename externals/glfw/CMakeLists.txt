cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "glfw")
message(STATUS "----------------------------------------------------------------")

set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE INTERNAL "Use MSVC runtime library DLL")

add_subdirectory(glfw)

cage_gen_module(glfw3 $<TARGET_LINKER_FILE:glfw> "")

