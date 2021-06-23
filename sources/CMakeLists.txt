cmake_minimum_required(VERSION 3.16)

########
# LIBS
########

file(GLOB_RECURSE cage-core-sources "libcore/*" "include/cage-core/*")
add_library(cage-core SHARED ${cage-core-sources})
target_include_directories(cage-core PUBLIC include)
target_compile_definitions(cage-core PRIVATE CAGE_CORE_EXPORT)
target_link_libraries(cage-core PRIVATE zlib libzstd_static simplefilewatcher utfcpp xsimd fastnoise hsluv png_static cage-jpeg tiff tiffxx tinyexr stb xatlas pmp dualmc dr_libs ogg vorbis vorbisfile vorbisenc samplerate)
target_link_libraries(cage-core PUBLIC OptickCore robin-hood-hashing plf)
if(WIN32)
	target_link_libraries(cage-core PRIVATE Ws2_32 Winmm)
else()
	target_link_libraries(cage-core PRIVATE m pthread)
	target_link_libraries(cage-core PUBLIC ${CMAKE_DL_LIBS})
endif()
if(MSVC)
	set_source_files_properties(libcore/marchingCubes.cpp PROPERTIES COMPILE_FLAGS "/wd4244 /wd4267") # it includes file with weird extension which does not honor warnings disabled by pragma
endif()
cage_ide_category(cage-core cage/libs)
cage_ide_sort_files(cage-core)

file(GLOB_RECURSE cage-engine-sources "libengine/*" "include/cage-engine/*")
add_library(cage-engine SHARED ${cage-engine-sources})
target_include_directories(cage-engine PUBLIC include)
target_compile_definitions(cage-engine PRIVATE CAGE_ENGINE_EXPORT)
target_link_libraries(cage-engine PRIVATE cubeb glfw)
target_link_libraries(cage-engine PUBLIC cage-core glad)
cage_ide_category(cage-engine cage/libs)
cage_ide_sort_files(cage-engine)

########
# TESTS
########

file(GLOB_RECURSE cage-test-core-sources "test-core/*")
add_executable(cage-test-core ${cage-test-core-sources})
target_link_libraries(cage-test-core cage-core glm)
cage_ide_category(cage-test-core cage/tests)
cage_ide_sort_files(cage-test-core)
cage_ide_working_dir_in_place(cage-test-core)

file(GLOB_RECURSE cage-test-ginnel-sources "test-ginnel/*")
add_executable(cage-test-ginnel ${cage-test-ginnel-sources})
target_link_libraries(cage-test-ginnel cage-core)
cage_ide_category(cage-test-ginnel cage/tests)
cage_ide_sort_files(cage-test-ginnel)
cage_ide_working_dir_in_place(cage-test-ginnel)

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

cage_define_tool(cage-asset-processor "asset-processor/*")
cage_define_tool(cage-asset-database "asset-database/*")
cage_define_tool(cage-asset-analyze "asset-analyze/*")
cage_define_tool(cage-image-channels "image-channels/*")
cage_define_tool(cage-image-atlas "image-atlas/*")
cage_define_tool(cage-image-convert "image-convert/*")

target_link_libraries(cage-asset-processor cage-engine assimp freetype lib_msdfgen)

add_dependencies(cage-asset-database cage-asset-processor)
add_dependencies(cage-asset-analyze cage-asset-processor)
