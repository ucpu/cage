cmake_minimum_required(VERSION 3.10)

project(cage LANGUAGES CXX C)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage start")
message(STATUS "----------------------------------------------------------------")

########
# INITIALIZATION
########

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(cage_all)

if(CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_BINARY_DIR)
	cage_default_init() # standalone build
else()
	cage_disable_source_build()
	cage_build_configuration()
endif()

add_subdirectory(externals EXCLUDE_FROM_ALL)

########
# LIBS
########

file(GLOB_RECURSE cage-core-sources "sources/libcore/*" "include/cage-core/*")
add_library(cage-core SHARED ${cage-core-sources})
target_include_directories(cage-core PUBLIC include)
target_compile_definitions(cage-core PRIVATE CAGE_CORE_EXPORT)
target_link_libraries(cage-core PRIVATE zlib simplefilewatcher png_static jpeg-static tiff tiffxx utfcpp xsimd fastnoise zip hsluv stb_dxt)
target_link_libraries(cage-core PUBLIC OptickCore robin-hood-hashing)
if(WIN32)
	target_link_libraries(cage-core PRIVATE Ws2_32 Winmm)
else()
	target_link_libraries(cage-core PRIVATE m pthread)
endif()
cage_ide_category(cage-core cage/libs)
cage_ide_sort_files(cage-core)

file(GLOB_RECURSE cage-engine-sources "sources/libengine/*" "include/cage-engine/*")
add_library(cage-engine SHARED ${cage-engine-sources})
target_include_directories(cage-engine PUBLIC include)
target_compile_definitions(cage-engine PRIVATE CAGE_ENGINE_EXPORT)
target_link_libraries(cage-engine PRIVATE libsoundio_shared ogg vorbis vorbisfile glfw)
target_link_libraries(cage-engine PUBLIC cage-core glad)
cage_ide_category(cage-engine cage/libs)
cage_ide_sort_files(cage-engine)

########
# TESTS
########

file(GLOB_RECURSE cage-test-core-sources "sources/test-core/*")
add_executable(cage-test-core ${cage-test-core-sources})
target_link_libraries(cage-test-core cage-core glm)
cage_ide_category(cage-test-core cage/tests)
cage_ide_sort_files(cage-test-core)
cage_ide_working_dir_in_place(cage-test-core)

file(GLOB_RECURSE cage-test-network-sources "sources/test-network/*")
add_executable(cage-test-network ${cage-test-network-sources})
target_link_libraries(cage-test-network cage-core)
cage_ide_category(cage-test-network cage/tests)
cage_ide_sort_files(cage-test-network)
cage_ide_working_dir_in_place(cage-test-network)

########
# TOOLS
########

macro(cage_define_tool target sourcePattern)
	file(GLOB_RECURSE ${target}-sources ${sourcePattern})
	add_executable(${target} ${${target}-sources})
	target_link_libraries(${target} cage-core)
	cage_ide_category(${target} cage/tools)
	cage_ide_sort_files(${target})
	cage_ide_working_dir_in_place(${target})
endmacro()

cage_define_tool(cage-asset-processor "sources/asset-processor/*")
cage_define_tool(cage-asset-database "sources/asset-database/*")
cage_define_tool(cage-asset-analyze "sources/asset-analyze/*")
cage_define_tool(cage-image-channels "sources/image-channels/*")
cage_define_tool(cage-image-atlas "sources/image-atlas/*")
cage_define_tool(cage-image-convert "sources/image-convert/*")

target_link_libraries(cage-asset-processor cage-engine assimp freetype dr_libs ogg vorbis vorbisfile vorbisenc lib_msdfgen)

add_dependencies(cage-asset-database cage-asset-processor)
add_dependencies(cage-asset-analyze cage-asset-processor)

########
# FINALIZATION
########

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")
