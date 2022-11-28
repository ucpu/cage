cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

option(ENABLE_SHARED "Build shared libraries" FALSE)
option(WITH_SIMD "Include SIMD extensions, if available for this platform" TRUE)
option(REQUIRE_SIMD "Generate a fatal error if SIMD extensions are not available for this platform (default is to fall back to a non-SIMD build)" TRUE)
option(WITH_CRT_DLL "windows C runtime dll" TRUE)

set(HAVE_THREAD_LOCAL TRUE)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(INSERT CMAKE_PREFIX_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/yasm/win64")
    else()
        list(INSERT CMAKE_PREFIX_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/yasm/win32")
    endif()
endif()

if(NOT CMAKE_INSTALL_DOCDIR)
	set(CMAKE_INSTALL_DOCDIR ${CMAKE_INSTALL_BINDIR})
endif()

#string(REPLACE "/LTCG" "" CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")

add_subdirectory(jpeg)

add_library(cage-jpeg INTERFACE)
target_link_libraries(cage-jpeg INTERFACE jpeg-static)
target_include_directories(cage-jpeg INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/jpeg")
target_include_directories(cage-jpeg INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/jpeg")

cage_gen_module(JPEG $<TARGET_LINKER_FILE:cage-jpeg> $<TARGET_PROPERTY:cage-jpeg,INTERFACE_INCLUDE_DIRECTORIES>)

add_library(JPEG::JPEG ALIAS cage-jpeg)
