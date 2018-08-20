cmake_minimum_required(VERSION 3.2)

message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

option(ENABLE_SHARED "Build shared libraries" FALSE)

if(APPLE)
	option(WITH_SIMD "Include SIMD extensions, if available for this platform" false)
endif()

if(NOT CMAKE_INSTALL_DOCDIR)
	set(CMAKE_INSTALL_DOCDIR ${CMAKE_INSTALL_BINDIR})
endif()

add_subdirectory(jpeg)

cage_gen_module(JPEG $<TARGET_LINKER_FILE:jpeg-static> "${CMAKE_CURRENT_SOURCE_DIR}/jpeg;${CMAKE_CURRENT_BINARY_DIR}/jpeg")
