cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

option(ENABLE_SHARED "Build shared libraries" FALSE)
option(WITH_SIMD "Include SIMD extensions, if available for this platform" FALSE)
option(WITH_CRT_DLL "windows C runtime dll" TRUE)

if(NOT CMAKE_INSTALL_DOCDIR)
	set(CMAKE_INSTALL_DOCDIR ${CMAKE_INSTALL_BINDIR})
endif()

add_subdirectory(jpeg)

target_include_directories(jpeg-static INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/jpeg")
target_include_directories(jpeg-static INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/jpeg")

cage_gen_module(JPEG $<TARGET_LINKER_FILE:jpeg-static> $<TARGET_PROPERTY:jpeg-static,INTERFACE_INCLUDE_DIRECTORIES>)
