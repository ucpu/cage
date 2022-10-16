cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "openxr-sdk")
message(STATUS "----------------------------------------------------------------")

set(DYNAMIC_LOADER OFF CACHE BOOL "Build the openxr loader as a .dll library" FORCE)
set(BUILD_API_LAYERS OFF CACHE BOOL "Build openxr API layers" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build openxr tests" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "Build openxr conformance tests" FORCE)
set(BUILD_WITH_SYSTEM_JSONCPP OFF CACHE BOOL "Use system jsoncpp instead of vendored source in openxr" FORCE)

add_subdirectory(OpenXR-SDK)
