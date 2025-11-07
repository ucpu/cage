message(STATUS "----------------------------------------------------------------")
message(STATUS "png")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

unset(CMAKE_C_STANDARD)
unset(CMAKE_CXX_STANDARD)

set(PNG_SHARED OFF CACHE INTERNAL "Build shared lib")
set(PNG_FRAMEWORK OFF CACHE INTERNAL "Build libpng as a framework bundle")
set(PNG_TESTS OFF CACHE INTERNAL "Build libpng tests")
set(PNG_TOOLS OFF CACHE INTERNAL "Build libpng tools")
set(SKIP_INSTALL_ALL ON CACHE INTERNAL "SKIP_INSTALL_ALL")

# prevent using awk for configuration - it causes spurious rebuilds of the entire build
find_program(AWK NAMES gawk awk)
set(AWK "")

add_subdirectory(png)

target_include_directories(png_static INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/png")
target_include_directories(png_static INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/png")
target_compile_definitions(png_static PRIVATE WIN32_LEAN_AND_MEAN VC_EXTRALEAN NOMINMAX)

cage_gen_module(PNG $<TARGET_LINKER_FILE:png_static> $<TARGET_PROPERTY:png_static,INTERFACE_INCLUDE_DIRECTORIES>)
