cmake_minimum_required(VERSION 3.2)

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
target_link_libraries(cage-core PRIVATE zlib simplefilewatcher png_static utfcpp xsimd fastnoise zip)
target_link_libraries(cage-core PUBLIC OptickCore robin-hood-hashing)
if(WIN32)
	target_link_libraries(cage-core PRIVATE Ws2_32 Winmm)
else()
	target_link_libraries(cage-core PRIVATE m pthread)
endif()
set_target_properties(cage-core PROPERTIES CXX_VISIBILITY_PRESET hidden)
cage_ide_category(cage-core cage)
cage_ide_sort_files(cage-core)
if(WIN32)
	target_compile_definitions(cage-core PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_options(cage-core PUBLIC /wd4251) # 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
endif()

########
# CAGE-TEST-CORE
########

file(GLOB_RECURSE cage-test-core-sources "sources/test-core/*")
add_executable(cage-test-core ${cage-test-core-sources})
target_link_libraries(cage-test-core cage-core glm)
cage_ide_category(cage-test-core cage)
cage_ide_sort_files(cage-test-core)
cage_ide_working_dir_in_place(cage-test-core)

########
# CAGE-TEST-NETWORK
########

file(GLOB_RECURSE cage-test-network-sources "sources/test-network/*")
add_executable(cage-test-network ${cage-test-network-sources})
target_link_libraries(cage-test-network cage-core)
cage_ide_category(cage-test-network cage)
cage_ide_sort_files(cage-test-network)
cage_ide_working_dir_in_place(cage-test-network)

########
# CAGE-ENGINE
########

file(GLOB_RECURSE cage-engine-sources "sources/libengine/*" "include/cage-engine/*")
add_library(cage-engine SHARED ${cage-engine-sources})
target_include_directories(cage-engine PUBLIC include)
target_link_libraries(cage-engine PRIVATE libsoundio_shared ogg vorbis vorbisfile glfw)
target_link_libraries(cage-engine PUBLIC cage-core glad)
set_target_properties(cage-engine PROPERTIES CXX_VISIBILITY_PRESET hidden)
cage_ide_category(cage-engine cage)
cage_ide_sort_files(cage-engine)

########
# CAGE-ASSET-PROCESSOR
########

file(GLOB_RECURSE cage-asset-processor-sources "sources/asset-processor/*")
add_executable(cage-asset-processor ${cage-asset-processor-sources})
target_link_libraries(cage-asset-processor cage-engine IL ILU assimp freetype dr_libs ogg vorbis vorbisfile vorbisenc lib_msdfgen)
cage_ide_category(cage-asset-processor cage)
cage_ide_sort_files(cage-asset-processor)
cage_ide_working_dir_in_place(cage-asset-processor)
if(WIN32)
	target_compile_definitions(cage-asset-processor PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

########
# CAGE-ASSET-DATABASE
########

file(GLOB_RECURSE cage-asset-database-sources "sources/asset-database/*")
add_executable(cage-asset-database ${cage-asset-database-sources})
target_link_libraries(cage-asset-database cage-core)
add_dependencies(cage-asset-database cage-asset-processor)
cage_ide_category(cage-asset-database cage)
cage_ide_sort_files(cage-asset-database)
cage_ide_working_dir_in_place(cage-asset-database)

########
# CAGE-IMAGE
########

file(GLOB_RECURSE cage-image-sources "sources/image/*")
add_executable(cage-image ${cage-image-sources})
target_link_libraries(cage-image cage-core)
cage_ide_category(cage-image cage)
cage_ide_sort_files(cage-image)
cage_ide_working_dir_in_place(cage-image)

########
# FINALIZATION
########

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")
