cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

include(ExternalProject)

set(CMAKE_DISABLE_SOURCE_CHANGES OFF) # ExternalProject_Add will attempt to create the source directory, which would fail

set(prefixpth "")
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(prefixpth "${CMAKE_CURRENT_SOURCE_DIR}/yasm/win64")
    else()
        set(prefixpth "${CMAKE_CURRENT_SOURCE_DIR}/yasm/win32")
    endif()
endif()

set(installdir "${CMAKE_CURRENT_BINARY_DIR}/jpeg_install_dir/$<CONFIG>")

ExternalProject_Add(jpeg-static
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jpeg"
	PREFIX "${CMAKE_CURRENT_BINARY_DIR}/jpeg_build_dir"
	CMAKE_CACHE_ARGS "-DENABLE_SHARED:BOOL=FALSE;-DWITH_SIMD:BOOL=TRUE;-DREQUIRE_SIMD:BOOL=TRUE;-DWITH_TURBOJPEG:BOOL=FALSE;-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=TRUE;-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE};-DCMAKE_PREFIX_PATH:PATH=${prefixpth};-DCMAKE_INSTALL_PREFIX:PATH=${installdir}"
	TEST_COMMAND ""
	EXCLUDE_FROM_ALL TRUE
	DEPENDS zlib
)

add_library(cage-jpeg INTERFACE)
add_dependencies(cage-jpeg jpeg-static)
if(MSVC)
	target_link_libraries(cage-jpeg INTERFACE "${installdir}/lib/jpeg-static.lib")
else()
	target_link_libraries(cage-jpeg INTERFACE "${installdir}/lib/libjpeg.a")
endif()
target_include_directories(cage-jpeg INTERFACE "${installdir}/include")

cage_gen_module(JPEG $<TARGET_LINKER_FILE:cage-jpeg> $<TARGET_PROPERTY:cage-jpeg,INTERFACE_INCLUDE_DIRECTORIES>)

add_library(JPEG::JPEG ALIAS cage-jpeg)
