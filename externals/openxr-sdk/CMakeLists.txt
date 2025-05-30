message(STATUS "----------------------------------------------------------------")
message(STATUS "openxr-sdk")
message(STATUS "----------------------------------------------------------------")

unset(CMAKE_C_STANDARD)
unset(CMAKE_CXX_STANDARD)

set(DYNAMIC_LOADER OFF CACHE INTERNAL "Build the openxr loader as a .dll library")
set(BUILD_WITH_SYSTEM_JSONCPP OFF CACHE INTERNAL "Use system jsoncpp instead of vendored source in openxr")

add_subdirectory(OpenXR-SDK)
