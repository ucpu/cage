cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cage itself")
message(STATUS "----------------------------------------------------------------")

########
# CORE
########

file(GLOB_RECURSE cage-core-sources "libcore/*" "include/cage-core/*")
add_library(cage-core SHARED ${cage-core-sources})
target_compile_features(cage-core PUBLIC cxx_std_${CMAKE_CXX_STANDARD})
target_include_directories(cage-core PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(cage-core PRIVATE zlib libzstd_static simplefilewatcher utfcpp fastnoise hsluv png_static cage-jpeg tiff tiffxx tinyexr stb xatlas pmp dualmc dr_libs ogg vorbis vorbisfile vorbisenc samplerate assimp bc7enc_rdo avir)
target_link_libraries(cage-core PUBLIC unordered_dense plf)
if(WIN32)
	target_link_libraries(cage-core PRIVATE Ws2_32 Winmm)
else()
	target_link_libraries(cage-core PRIVATE m pthread)
	target_link_libraries(cage-core PUBLIC ${CMAKE_DL_LIBS})
endif()
if(MSVC)
	target_compile_definitions(cage-core PUBLIC "CAGE_API_EXPORT=__declspec(dllexport)")
	target_compile_definitions(cage-core PUBLIC "CAGE_API_IMPORT=__declspec(dllimport)")
	target_compile_definitions(cage-core PUBLIC "CAGE_FORCE_INLINE=__forceinline")
else()
	target_compile_definitions(cage-core PUBLIC "CAGE_API_EXPORT=[[gnu::visibility(\"default\")]]")
	target_compile_definitions(cage-core PUBLIC "CAGE_API_IMPORT=[[gnu::visibility(\"default\")]]")
	target_compile_definitions(cage-core PUBLIC "CAGE_FORCE_INLINE=[[gnu::always_inline]]inline")
endif()
if(WIN32)
	target_compile_definitions(cage-core PUBLIC "CAGE_SYSTEM_WINDOWS")
elseif(APPLE)
	target_compile_definitions(cage-core PUBLIC "CAGE_SYSTEM_MAC")
else()
	target_compile_definitions(cage-core PUBLIC "CAGE_SYSTEM_LINUX")
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	target_compile_definitions(cage-core PUBLIC "CAGE_ARCHITECTURE_64")
else()
	target_compile_definitions(cage-core PUBLIC "CAGE_ARCHITECTURE_32")
endif()
target_compile_definitions(cage-core PUBLIC "$<$<CONFIG:DEBUG>:CAGE_DEBUG>")
target_compile_definitions(cage-core PUBLIC "$<$<CONFIG:DEBUG>:CAGE_DEBUG_BOOL=true>")
target_compile_definitions(cage-core PUBLIC "$<$<NOT:$<CONFIG:DEBUG>>:CAGE_DEBUG_BOOL=false>")
option(cage_assert_in_release_enabled "enable runtime asserts in non-debug builds" OFF)
if(cage_assert_in_release_enabled)
	target_compile_definitions(cage-core PUBLIC "CAGE_ASSERT_ENABLED")
else()
	target_compile_definitions(cage-core PUBLIC "$<$<CONFIG:DEBUG>:CAGE_ASSERT_ENABLED>")
endif()
option(cage_profiling_enabled "enable compiling-in instructions for profiling" ON)
if(cage_profiling_enabled)
	target_compile_definitions(cage-core PUBLIC CAGE_PROFILING_ENABLED)
	cage_embed_file(cage-core "profiling.htm" profiling_htm)
	target_sources(cage-core PRIVATE "profiling.htm")
endif()
if(MSVC)
	set_source_files_properties(libcore/mesh/marchingCubes.cpp PROPERTIES COMPILE_FLAGS "/wd4244 /wd4267") # it includes file with weird extension which does not honor warnings disabled by pragma
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(cage-core PUBLIC "-Wno-assume") # __builtin_assume has side effects that are discarded
endif()
try_compile(sourceLocationAvailable "${CMAKE_CURRENT_BINARY_DIR}/tryCompile" SOURCES "${CMAKE_CURRENT_LIST_DIR}/tryCompileSourceLocation.cpp" OUTPUT_VARIABLE sourceLocationMessages)
if (NOT sourceLocationAvailable)
	target_compile_definitions(cage-core PUBLIC GCHL_DUMMY_SOURCE_LOCATION)
	message(STATUS "source location not available:")
	message(STATUS "${sourceLocationMessages}")
endif()
cage_target_api_macro(cage-core CAGE_CORE_API)
cage_ide_category(cage-core cage/libs)
cage_ide_sort_files(cage-core)

########
# ENGINE
########

file(GLOB_RECURSE cage-engine-sources "libengine/*" "include/cage-engine/*")
add_library(cage-engine SHARED ${cage-engine-sources})
target_link_libraries(cage-engine PRIVATE cubeb glfw openxr_loader)
target_link_libraries(cage-engine PUBLIC cage-core glad)
if(WIN32)
	set_source_files_properties(libengine/virtualReality/platforms.cpp PROPERTIES COMPILE_DEFINITIONS "XR_USE_PLATFORM_WIN32")
else()
	# todo
endif()
file(GLOB_RECURSE controller-bindings RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "controller-bindings/*")
set(index 0)
foreach(cb IN ITEMS ${controller-bindings})
	cage_embed_file(cage-engine "${cb}" "controller_binding_${index}")
	target_sources(cage-engine PRIVATE "${CMAKE_CURRENT_LIST_DIR}/${cb}")
	math(EXPR index "${index}+1")
endforeach()
cage_target_api_macro(cage-engine CAGE_ENGINE_API)
cage_ide_category(cage-engine cage/libs)
cage_ide_sort_files(cage-engine)

########
# SIMPLE
########

file(GLOB_RECURSE cage-simple-sources "libsimple/*" "include/cage-simple/*")
add_library(cage-simple STATIC ${cage-simple-sources})
target_link_libraries(cage-simple PUBLIC cage-engine)
cage_ide_category(cage-simple cage/libs)
cage_ide_sort_files(cage-simple)

########
# DATA
########

if(MSVC)
	file(GLOB_RECURSE cage-data-sources "../data/*")
	file(GLOB_RECURSE cage-schemes-sources "../schemes/*")
	add_library(cage-data INTERFACE ${cage-data-sources} ${cage-schemes-sources})
	cage_ide_category(cage-data cage/libs)
	cage_ide_sort_files(cage-data "${CMAKE_CURRENT_LIST_DIR}/..")
endif()

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

file(GLOB_RECURSE cage-test-assets-sources "test-assets/*")
add_executable(cage-test-assets ${cage-test-assets-sources})
target_link_libraries(cage-test-assets cage-core)
cage_ide_category(cage-test-assets cage/tests)
cage_ide_sort_files(cage-test-assets)
cage_ide_working_dir_in_place(cage-test-assets)

########
# TOOLS
########

set(cage_tools_targets_list)
macro(cage_define_tool target sourcePattern)
	file(GLOB_RECURSE ${target}-sources ${sourcePattern})
	add_executable(${target} ${${target}-sources})
	target_link_libraries(${target} cage-core)
	cage_ide_category(${target} cage/tools)
	cage_ide_sort_files(${target})
	cage_ide_working_dir_in_place(${target})
	list(APPEND cage_tools_targets_list ${target})
endmacro()

cage_define_tool(cage-asset-analyze "asset-analyze/*")
cage_define_tool(cage-asset-database "asset-database/*")
cage_define_tool(cage-asset-processor "asset-processor/*")
cage_define_tool(cage-image-convert "image-convert/*")
cage_define_tool(cage-image-channels "image-channels/*")
cage_define_tool(cage-image-info "image-info/*")
cage_define_tool(cage-image-resize "image-resize/*")
cage_define_tool(cage-image-untile "image-untile/*")
cage_define_tool(cage-mesh-info "mesh-info/*")

target_link_libraries(cage-asset-processor cage-engine freetype lib_msdfgen)

add_dependencies(cage-asset-database cage-asset-processor)
add_dependencies(cage-asset-analyze cage-asset-processor)

########
# INSTALL
########

option(cage_install_enabled "enable installing cage" ON)
include(GNUInstallDirs)
if (cage_install_enabled)
	install(TARGETS unordered_dense plf glad EXPORT CageConfig)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../externals/unordered_dense/unordered_dense/include/ankerl/unordered_dense.h" DESTINATION include)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../externals/plf/plf_colony/plf_colony.h" DESTINATION include)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../externals/plf/plf_list/plf_list.h" DESTINATION include)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../externals/plf/plf_stack/plf_stack.h" DESTINATION include)
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../externals/glad/glad/include" DESTINATION .)
	install(TARGETS cage-core cage-engine cage-simple ${cage_tools_targets_list} EXPORT CageConfig)
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" DESTINATION .)
	install(EXPORT CageConfig DESTINATION cmake)
endif()
