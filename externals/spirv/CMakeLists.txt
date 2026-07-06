message(STATUS "----------------------------------------------------------------")
message(STATUS "spirv")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

set(CMAKE_DISABLE_FIND_PACKAGE_Python3 OFF)

set(SPIRV_SKIP_EXECUTABLES ON CACHE INTERNAL "SPIRV_SKIP_EXECUTABLES")
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE INTERNAL "SKIP_SPIRV_TOOLS_INSTALL")
set(ENABLE_SPIRV_TOOLS_INSTALL OFF CACHE INTERNAL "ENABLE_SPIRV_TOOLS_INSTALL")
set(SPIRV_WERROR OFF CACHE INTERNAL "SPIRV_WERROR")

set(GLSLANG_ENABLE_INSTALL OFF CACHE INTERNAL "GLSLANG_ENABLE_INSTALL")
set(GLSLANG_TESTS OFF CACHE INTERNAL "GLSLANG_TESTS")
set(ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "ENABLE_GLSLANG_BINARIES")
set(ENABLE_HLSL OFF CACHE INTERNAL "ENABLE_HLSL")
set(ENABLE_EXCEPTIONS ON CACHE INTERNAL "ENABLE_EXCEPTIONS")

add_subdirectory(headers)
add_subdirectory(tools)
add_subdirectory(glslang)

add_library(cage_spirv INTERFACE)
target_link_libraries(cage_spirv INTERFACE spirv-tools-static glslang::glslang)
add_library(cage::spirv ALIAS cage_spirv)
