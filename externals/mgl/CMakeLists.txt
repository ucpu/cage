message(STATUS "----------------------------------------------------------------")
message(STATUS "mgl")
message(STATUS "----------------------------------------------------------------")

if(NOT APPLE)
	message(STATUS "mgl is used on macos only - skipping")
	return()
endif()

set(CMAKE_DISABLE_FIND_PACKAGE_Python3 OFF)

add_subdirectory(mgl)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "mgl/MGL/include")
