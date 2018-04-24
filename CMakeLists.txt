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
cage_disable_source_build()
cage_build_configuration()

add_subdirectory(externals EXCLUDE_FROM_ALL)

########
# CAGE-CORE
########

file(GLOB_RECURSE cage-core-sources "sources/libcore/*" "include/cage-core/*")
add_library(cage-core SHARED ${cage-core-sources})
target_include_directories(cage-core PUBLIC include)
target_link_libraries(cage-core zlib simplefilewatcher png_static enet utfcpp)
if(WIN32)
	target_link_libraries(cage-core Ws2_32 Winmm)
else()
	target_link_libraries(cage-core m pthread)
endif()
set_target_properties(cage-core PROPERTIES CXX_VISIBILITY_PRESET hidden)
cage_ide_groups(cage-core cage)
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
cage_ide_groups(cage-test-core cage)
cage_ide_working_dir_in_place(cage-test-core)

########
# CAGE-CLIENT
########

file(GLOB_RECURSE cage-client-sources "sources/libclient/*" "include/cage-client/*")
add_library(cage-client SHARED ${cage-client-sources})
target_include_directories(cage-client PUBLIC include)
target_link_libraries(cage-client cage-core glad libsoundio_shared ogg vorbis vorbisfile glfw)
set_target_properties(cage-client PROPERTIES CXX_VISIBILITY_PRESET hidden)
cage_ide_groups(cage-client cage)

########
# CAGE-ASSET-PROCESSOR
########

file(GLOB_RECURSE cage-asset-processor-sources "sources/asset-processor/*")
add_executable(cage-asset-processor ${cage-asset-processor-sources})
target_link_libraries(cage-asset-processor cage-client IL ILU assimp freetype dr_libs vorbisenc)
cage_ide_groups(cage-asset-processor cage)
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
cage_ide_groups(cage-asset-database cage)
cage_ide_working_dir_in_place(cage-asset-database)

########
# CAGE-ASSET-GENERATOR
########

file(GLOB_RECURSE cage-asset-generator-sources "sources/asset-generator/*")
add_executable(cage-asset-generator ${cage-asset-generator-sources})
target_link_libraries(cage-asset-generator cage-client)
add_dependencies(cage-asset-generator cage-asset-processor)
cage_ide_groups(cage-asset-generator cage)
cage_ide_working_dir_in_place(cage-asset-generator)

########
# FINALIZATION
########

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")
