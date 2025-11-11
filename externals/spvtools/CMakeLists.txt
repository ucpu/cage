message(STATUS "----------------------------------------------------------------")
message(STATUS "spvtools")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

set(CMAKE_DISABLE_FIND_PACKAGE_Python3 OFF)

set(SKIP_SPIRV_TOOLS_INSTALL OFF CACHE INTERNAL "")
set(SPIRV_TOOLS_BUILD_STATIC ON CACHE INTERNAL "")
set(SPIRV_SKIP_EXECUTABLES ON CACHE INTERNAL "")
set(SPIRV_SKIP_TESTS ON CACHE INTERNAL "")

add_subdirectory(spvtools)

add_library(cage_spvtools INTERFACE)
target_link_libraries(cage_spvtools INTERFACE $<TARGET_LINKER_FILE:SPIRV-Tools>)
target_link_libraries(cage_spvtools INTERFACE $<TARGET_LINKER_FILE:SPIRV-Tools-opt>)
target_link_libraries(cage_spvtools INTERFACE $<TARGET_LINKER_FILE:SPIRV-Tools-link>)
target_include_directories(cage_spvtools INTERFACE $<TARGET_PROPERTY:SPIRV-Tools,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(cage_spvtools INTERFACE spvtools)
target_include_directories(cage_spvtools INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/spvtools")
add_dependencies(cage_spvtools SPIRV-Tools SPIRV-Tools-opt SPIRV-Tools-link)
add_library(cage::spvtools ALIAS cage_spvtools)
