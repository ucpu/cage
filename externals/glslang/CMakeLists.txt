message(STATUS "----------------------------------------------------------------")
message(STATUS "glslang")
message(STATUS "----------------------------------------------------------------")

set(BUILD_EXTERNAL OFF CACHE INTERNAL "Build external dependencies in /External" FORCE)
set(GLSLANG_TESTS OFF CACHE INTERNAL  "Enable glslang testing" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE INTERNAL "Builds glslang" FORCE)
set(ENABLE_HLSL OFF CACHE INTERNAL "Enables HLSL input support" FORCE)

add_subdirectory(glslang)

add_library(cage_glslang INTERFACE)
target_link_libraries(cage_glslang INTERFACE $<TARGET_LINKER_FILE:glslang>)
target_link_libraries(cage_glslang INTERFACE $<TARGET_LINKER_FILE:glslang-default-resource-limits>)
target_include_directories(cage_glslang INTERFACE $<TARGET_PROPERTY:glslang,INTERFACE_INCLUDE_DIRECTORIES>)
add_dependencies(cage_glslang glslang glslang-default-resource-limits)
add_library(cage::glslang ALIAS cage_glslang)
