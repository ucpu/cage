message(STATUS "----------------------------------------------------------------")
message(STATUS "glslang")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

set(CMAKE_DISABLE_FIND_PACKAGE_Python3 OFF)

set(BUILD_EXTERNAL OFF CACHE INTERNAL "")
set(GLSLANG_TESTS OFF CACHE INTERNAL  "")
set(ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "")
set(ENABLE_HLSL OFF CACHE INTERNAL "")

add_subdirectory(glslang)

add_library(cage_glslang INTERFACE)
target_link_libraries(cage_glslang INTERFACE $<TARGET_LINKER_FILE:glslang>)
target_link_libraries(cage_glslang INTERFACE $<TARGET_LINKER_FILE:glslang-default-resource-limits>)
target_include_directories(cage_glslang INTERFACE $<TARGET_PROPERTY:glslang,INTERFACE_INCLUDE_DIRECTORIES>)
add_dependencies(cage_glslang glslang glslang-default-resource-limits)
add_library(cage::glslang ALIAS cage_glslang)
