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
target_link_libraries(cage-core PRIVATE zlib libzstd_static simplefilewatcher fastnoise hsluv png_static cage-jpeg tiff tiffxx tinyexr stb xatlas pmp dualmc dr_libs ogg vorbis vorbisfile vorbisenc samplerate assimp bc7enc_rdo avir quickhull iwasm_static uni-algo cpr absl_log)
target_link_libraries(cage-core PUBLIC svector unordered_dense plf)
if (cage_use_steam_sockets)
	target_link_libraries(cage-core PRIVATE GameNetworkingSockets::GameNetworkingSockets)
	target_compile_definitions(cage-core PUBLIC "CAGE_USE_STEAM_SOCKETS")
endif()
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
target_compile_definitions(cage-core PUBLIC "$<$<CONFIG:RELEASE>:CAGE_DEPLOY>")
if(cage_assert_in_release_enabled)
	target_compile_definitions(cage-core PUBLIC "CAGE_ASSERT_ENABLED")
else()
	target_compile_definitions(cage-core PUBLIC "$<$<CONFIG:DEBUG>:CAGE_ASSERT_ENABLED>")
endif()
if(cage_profiling_enabled)
	target_compile_definitions(cage-core PUBLIC CAGE_PROFILING_ENABLED)
	cage_embed_file(cage-core "profiling.htm" profiling_htm)
	target_sources(cage-core PRIVATE "profiling.htm")
endif()
if(MSVC)
	set_source_files_properties(libcore/mesh/marchingCubes.cpp PROPERTIES COMPILE_FLAGS "/wd4244 /wd4267") # it includes file with weird extension which does not honor warnings disabled by pragma
endif()
target_precompile_headers(cage-core PRIVATE "<cage-core/core.h>")
target_precompile_headers(cage-core INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:$<BUILD_INTERFACE:<cage-core/core.h$<ANGLE-R>>>")
cage_target_api_macro(cage-core CAGE_CORE_API)
cage_ide_category(cage-core cage/libs)
cage_ide_sort_files(cage-core)

########
# ENGINE
########

file(GLOB_RECURSE cage-engine-sources "libengine/*" "include/cage-engine/*")
add_library(cage-engine SHARED ${cage-engine-sources})
target_link_libraries(cage-engine PRIVATE cubeb glfw openxr_loader freetype harfbuzz SheenBidi cage::glslang cage::spvtools)
target_link_libraries(cage-engine PUBLIC cage-core cage::dawn)
file(GLOB_RECURSE controller-bindings RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "controller-bindings/*")
set(index 0)
foreach(cb IN ITEMS ${controller-bindings})
	cage_embed_file(cage-engine "${cb}" "controller_binding_${index}")
	target_sources(cage-engine PRIVATE "${CMAKE_CURRENT_LIST_DIR}/${cb}")
	math(EXPR index "${index}+1")
endforeach()
target_precompile_headers(cage-engine PRIVATE "<cage-engine/core.h>")
target_precompile_headers(cage-engine INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:$<BUILD_INTERFACE:<cage-engine/core.h$<ANGLE-R>>>")
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

file(GLOB cage-test-core-sources LIST_DIRECTORIES OFF "test-core/*")
add_executable(cage-test-core ${cage-test-core-sources})
target_link_libraries(cage-test-core cage-core glm)
cage_ide_category(cage-test-core cage/tests)
cage_ide_sort_files(cage-test-core)
cage_ide_working_dir_in_place(cage-test-core)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	target_compile_options(cage-test-core PRIVATE "-Wno-unused-variable")
	target_compile_options(cage-test-core PRIVATE "-Wno-unused-but-set-variable")
endif()
file(GLOB_RECURSE wasm-files RELATIVE "${CMAKE_CURRENT_LIST_DIR}/test-core/wasm" "*.wasm")
foreach(wf IN ITEMS ${wasm-files})
	string(REPLACE "." "_" fn ${wf})
	cage_embed_file(cage-test-core "test-core/wasm/${wf}" "${fn}")
endforeach()

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

file(GLOB_RECURSE cage-test-entities-sources "test-entities/*")
add_executable(cage-test-entities ${cage-test-entities-sources})
target_link_libraries(cage-test-entities cage-core)
cage_ide_category(cage-test-entities cage/tests)
cage_ide_sort_files(cage-test-entities)
cage_ide_working_dir_in_place(cage-test-entities)

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
cage_define_tool(cage-files "files/*")
cage_define_tool(cage-image-channels "image-channels/*")
cage_define_tool(cage-image-convert "image-convert/*")
cage_define_tool(cage-image-info "image-info/*")
cage_define_tool(cage-image-resize "image-resize/*")
cage_define_tool(cage-image-untile "image-untile/*")
cage_define_tool(cage-mesh-convert "mesh-convert/*")
cage_define_tool(cage-mesh-info "mesh-info/*")

target_link_libraries(cage-asset-processor cage-engine freetype lib_msdfgen)

add_dependencies(cage-asset-database cage-asset-processor)
add_dependencies(cage-asset-analyze cage-asset-processor)
