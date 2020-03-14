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
# CAGE-CORE
########

file(GLOB_RECURSE cage-core-sources "sources/libcore/*" "include/cage-core/*")
add_library(cage-core SHARED ${cage-core-sources})
target_include_directories(cage-core PUBLIC include)
target_compile_definitions(cage-core PRIVATE CAGE_CORE_EXPORT)
target_link_libraries(cage-core PRIVATE zlib simplefilewatcher png_static jpeg-static tiff tiffxx utfcpp xsimd fastnoise zip)
target_link_libraries(cage-core PUBLIC OptickCore robin-hood-hashing)
if(WIN32)
	target_link_libraries(cage-core PRIVATE Ws2_32 Winmm)
else()
	target_link_libraries(cage-core PRIVATE m pthread)
endif()
cage_ide_category(cage-core cage/libs)
cage_ide_sort_files(cage-core)

########
# CAGE-TEST-CORE
########

file(GLOB_RECURSE cage-test-core-sources "sources/test-core/*")
add_executable(cage-test-core ${cage-test-core-sources})
target_link_libraries(cage-test-core cage-core glm)
cage_ide_category(cage-test-core cage/tests)
cage_ide_sort_files(cage-test-core)
cage_ide_working_dir_in_place(cage-test-core)

########
# CAGE-TEST-NETWORK
########

file(GLOB_RECURSE cage-test-network-sources "sources/test-network/*")
add_executable(cage-test-network ${cage-test-network-sources})
target_link_libraries(cage-test-network cage-core)
cage_ide_category(cage-test-network cage/tests)
cage_ide_sort_files(cage-test-network)
cage_ide_working_dir_in_place(cage-test-network)

########
# CAGE-ENGINE
########

file(GLOB_RECURSE cage-engine-sources "sources/libengine/*" "include/cage-engine/*")
add_library(cage-engine SHARED ${cage-engine-sources})
target_include_directories(cage-engine PUBLIC include)
target_compile_definitions(cage-engine PRIVATE CAGE_ENGINE_EXPORT)
target_link_libraries(cage-engine PRIVATE libsoundio_shared ogg vorbis vorbisfile glfw)
target_link_libraries(cage-engine PUBLIC cage-core glad)
cage_ide_category(cage-engine cage/libs)
cage_ide_sort_files(cage-engine)

########
# CAGE-ASSET-PROCESSOR
########

file(GLOB_RECURSE cage-asset-processor-sources "sources/asset-processor/*")
add_executable(cage-asset-processor ${cage-asset-processor-sources})
target_link_libraries(cage-asset-processor cage-engine assimp freetype dr_libs ogg vorbis vorbisfile vorbisenc lib_msdfgen)
cage_ide_category(cage-asset-processor cage/tools)
cage_ide_sort_files(cage-asset-processor)
cage_ide_working_dir_in_place(cage-asset-processor)

########
# CAGE-ASSET-DATABASE
########

file(GLOB_RECURSE cage-asset-database-sources "sources/asset-database/*")
add_executable(cage-asset-database ${cage-asset-database-sources})
target_link_libraries(cage-asset-database cage-core)
add_dependencies(cage-asset-database cage-asset-processor)
cage_ide_category(cage-asset-database cage/tools)
cage_ide_sort_files(cage-asset-database)
cage_ide_working_dir_in_place(cage-asset-database)

########
# CAGE-IMAGE-CHANNELS
########

file(GLOB_RECURSE cage-image-channels-sources "sources/image-channels/*")
add_executable(cage-image-channels ${cage-image-channels-sources})
target_link_libraries(cage-image-channels cage-core)
cage_ide_category(cage-image-channels cage/tools)
cage_ide_sort_files(cage-image-channels)
cage_ide_working_dir_in_place(cage-image-channels)

########
# CAGE-IMAGE-ATLAS
########

file(GLOB_RECURSE cage-image-atlas-sources "sources/image-atlas/*")
add_executable(cage-image-atlas ${cage-image-atlas-sources})
target_link_libraries(cage-image-atlas cage-core)
cage_ide_category(cage-image-atlas cage/tools)
cage_ide_sort_files(cage-image-atlas)
cage_ide_working_dir_in_place(cage-image-atlas)

########
# FINALIZATION
########

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")
