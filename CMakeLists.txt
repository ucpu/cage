cmake_minimum_required(VERSION 3.2)

project(cage LANGUAGES CXX)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage start")
message(STATUS "----------------------------------------------------------------")

########
# INITIALIZATION
########

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(all)
disable_source_build()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 11)
if(WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(externals EXCLUDE_FROM_ALL)

########
# OPTIONS
########

option(CAGE-CORE_BUILD_TESTS "Enable building tests for cage-core." ON)
option(CAGE-CLIENT_BUILD_TESTS "Enable building tests for cage-client." ON)
option(CAGE-CLIENT_BUILD_TOOLS "Enable building tools for cage-client." ON)

########
# CAGE-CORE
########

file(GLOB_RECURSE cage-core-sources "sources/cage-core/*" "include/cage-core/*")
add_library(cage-core SHARED ${cage-core-sources})
target_include_directories(cage-core PUBLIC include)
target_link_libraries(cage-core zlib simplefilewatcher lodepng enet utfcpp)
if(WIN32)
	target_link_libraries(cage-core Ws2_32 Winmm)
else(WIN32)
	target_link_libraries(cage-core m pthread)
endif(WIN32)
set_ide_groups(cage-core cage)

########
# CAGE-TEST-CORE
########

if(CAGE-CORE_BUILD_TESTS)
	file(GLOB_RECURSE cage-test-core-sources "sources/cage-test-core/*")
	add_executable(cage-test-core ${cage-test-core-sources})
	target_link_libraries(cage-test-core cage-core glm)
	set_ide_groups(cage-test-core cage/tests)
	set_ide_working_dir_in_place(cage-test-core)
endif(CAGE-CORE_BUILD_TESTS)

########
# CAGE-CLIENT
########

file(GLOB_RECURSE cage-client-sources "sources/cage-client/*" "include/cage-client/*")
add_library(cage-client SHARED ${cage-client-sources})
target_include_directories(cage-client PUBLIC include)
target_link_libraries(cage-client cage-core glad libsoundio_shared ogg vorbis vorbisfile glfw)
set_ide_groups(cage-client cage)

########
# CAGE-ASSET-PROCESSOR
########

if(CAGE-CLIENT_BUILD_TOOLS)
	file(GLOB_RECURSE cage-asset-processor-sources "sources/cage-asset-processor/*")
	add_executable(cage-asset-processor ${cage-asset-processor-sources})
	target_link_libraries(cage-asset-processor cage-client IL ILU assimp freetype dr_libs vorbisenc)
	set_ide_groups(cage-asset-processor cage)
	set_ide_working_dir_in_place(cage-asset-processor)
endif(CAGE-CLIENT_BUILD_TOOLS)

########
# CAGE-ASSET-DATABASE
########

if(CAGE-CLIENT_BUILD_TOOLS)
	file(GLOB_RECURSE cage-asset-database-sources "sources/cage-asset-database/*")
	add_executable(cage-asset-database ${cage-asset-database-sources})
	target_link_libraries(cage-asset-database cage-core)
	add_dependencies(cage-asset-database cage-asset-processor)
	set_ide_groups(cage-asset-database cage)
	set_ide_working_dir_in_place(cage-asset-database)
endif(CAGE-CLIENT_BUILD_TOOLS)

########
# CAGE-ASSET-GENERATOR
########

if(CAGE-CLIENT_BUILD_TOOLS)
	file(GLOB_RECURSE cage-asset-generator-sources "sources/cage-asset-generator/*")
	add_executable(cage-asset-generator ${cage-asset-generator-sources})
	target_link_libraries(cage-asset-generator cage-client)
	add_dependencies(cage-asset-generator cage-asset-processor)
	set_ide_groups(cage-asset-generator cage)
	set_ide_working_dir_in_place(cage-asset-generator)
endif(CAGE-CLIENT_BUILD_TOOLS)

########
# CAGE-TESTS-CLIENT
########

if(CAGE-CLIENT_BUILD_TESTS)
	file(GLOB cage-tests-client-list "sources/cage-tests-client/*" "sources/cage-tests-engine/*")
	foreach(test ${cage-tests-client-list})
		string(REPLACE ".cpp" "" name ${test})
		get_filename_component(name ${name} NAME)
		add_executable(${name} ${test})
		target_link_libraries(${name} cage-client)
		set_ide_groups(${name} cage/tests)
		set_ide_working_dir_in_place(${name})
	endforeach()
endif(CAGE-CLIENT_BUILD_TESTS)

########
# FINALIZATION
########

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage done")
message(STATUS "----------------------------------------------------------------")
