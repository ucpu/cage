cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "game networking sockets")
message(STATUS "----------------------------------------------------------------")

set(BUILD_STATIC_LIB ON CACHE INTERNAL "Build static lib")
set(BUILD_SHARED_LIB OFF CACHE INTERNAL "Build shared lib")
set(Protobuf_USE_STATIC_LIBS ON CACHE INTERNAL "static protobuf")
if(WIN32)
	set(USE_CRYPTO "BCrypt" CACHE INTERNAL "use BCrypt")
else()
	set(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig OFF) # pkgconfig is used inside openssl
endif()

add_subdirectory(GameNetworkingSockets)

set_target_properties(GameNetworkingSockets_s PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD})
