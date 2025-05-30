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

set_target_properties(GameNetworkingSockets_s PROPERTIES
	CXX_STANDARD ${CMAKE_CXX_STANDARD}
	UNITY_BUILD ON
	UNITY_BUILD_BATCH_SIZE 100
)
file(GLOB_RECURSE donna_files
	"GameNetworkingSockets/src/external/ed25519-donna/*.c"
	"GameNetworkingSockets/src/external/curve25519-donna/*.c"
)
set_source_files_properties(
	${donna_files}
	"${CMAKE_CURRENT_BINARY_DIR}/GameNetworkingSockets/src/steamnetworkingsockets_messages.pb.cc"
	"${CMAKE_CURRENT_BINARY_DIR}/GameNetworkingSockets/src/steamnetworkingsockets_messages_certs.pb.cc"
	"${CMAKE_CURRENT_BINARY_DIR}/GameNetworkingSockets/src/steamnetworkingsockets_messages_udp.pb.cc"
	"GameNetworkingSockets/src/steamnetworkingsockets/clientlib/steamnetworkingsockets_lowlevel.cpp"
	TARGET_DIRECTORY GameNetworkingSockets_s
	PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

set_source_files_properties(
	${donna_files}
	TARGET_DIRECTORY GameNetworkingSockets_s
	PROPERTIES COMPILE_DEFINITIONS "WIN32_LEAN_AND_MEAN;VC_EXTRALEAN;NOMINMAX"
)
