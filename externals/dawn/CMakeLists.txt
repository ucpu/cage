message(STATUS "----------------------------------------------------------------")
message(STATUS "dawn")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

function(replace_in_file target_file)
	# remaining arguments must be pairs: PATTERN REPLACEMENT
	set(pairs ${ARGN})
	list(LENGTH pairs pair_len)
	file(READ "${target_file}" _content)
	math(EXPR last "${pair_len} - 1")
	foreach(i RANGE 0 ${last} 2)
		math(EXPR j "${i} + 1")
		list(GET pairs ${i} pattern)
		list(GET pairs ${j} replacement)
		string(REGEX REPLACE "${pattern}" "${replacement}" _content "${_content}")
	endforeach()
	set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
	file(CONFIGURE OUTPUT "${target_file}" CONTENT "${_content}" @ONLY)
	set(CMAKE_DISABLE_SOURCE_CHANGES ON)
endfunction()
replace_in_file(
	"${CMAKE_CURRENT_LIST_DIR}/dawn/CMakeLists.txt"
	"set\\(CMAKE_DEBUG_POSTFIX" "set(_CMAKE_DEBUG_POSTFIX"
	"set\\(IS_DEBUG_BUILD 1\\)" "set(IS_DEBUG_BUILD 0)"
)
replace_in_file(
	"${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/CMakeLists.txt"
	"NOT TARGET glslang AND \\(TINT_BUILD_GLSL_WRITER OR TINT_BUILD_GLSL_VALIDATOR\\) AND TINT_BUILD_CMD_TOOLS" "ON"
)
replace_in_file(
	"${CMAKE_CURRENT_LIST_DIR}/dawn/src/dawn/common/Constants.h"
	"kMaxSampledTexturesPerShaderStage = 16;" "kMaxSampledTexturesPerShaderStage = 32;"
)


set(CMAKE_DISABLE_FIND_PACKAGE_Python3 OFF)


if(WIN32)
	set(metal OFF)
	set(vulkan ON)
	set(linux OFF)
elseif(APPLE)
	set(metal ON)
	set(vulkan OFF)
	set(linux OFF)
else()
	set(metal OFF)
	set(vulkan ON)
	set(linux ON)
endif()

# dawn
set(DAWN_ENABLE_INSTALL OFF CACHE INTERNAL "Enable install step for Dawn libraries")
set(DAWN_ENABLE_D3D11 OFF CACHE INTERNAL "Enable compilation of the D3D11 backend")
set(DAWN_ENABLE_D3D12 OFF CACHE INTERNAL "Enable compilation of the D3D12 backend")
set(DAWN_ENABLE_METAL ${metal} CACHE INTERNAL "Enable compilation of the Metal backend")
set(DAWN_ENABLE_NULL OFF CACHE INTERNAL "Enable compilation of the Null backend")
set(DAWN_ENABLE_WEBGPU_ON_WEBGPU OFF CACHE INTERNAL "Enable compilation of the WebGPU backend")
set(DAWN_ENABLE_DESKTOP_GL OFF CACHE INTERNAL "Enable compilation of the OpenGL backend")
set(DAWN_ENABLE_OPENGLES OFF CACHE INTERNAL "Enable compilation of the OpenGL ES backend")
set(DAWN_ENABLE_VULKAN ${vulkan} CACHE INTERNAL "Enable compilation of the Vulkan backend")
set(DAWN_ENABLE_SPIRV_VALIDATION ${cage_validate_graphics} CACHE INTERNAL "Enable validation of SPIR-V")
set(DAWN_FORCE_SYSTEM_COMPONENT_LOAD ON CACHE INTERNAL "Allow system component fallback")
set(DAWN_USE_WAYLAND ${linux} CACHE INTERNAL "Enable support for Wayland surface")
set(DAWN_USE_X11 ${linux} CACHE INTERNAL "Enable support for X11 surface")
set(DAWN_USE_GLFW ON CACHE INTERNAL "Enable compilation of the GLFW windowing utils")
set(DAWN_USE_WINDOWS_UI OFF CACHE INTERNAL "Enable support for Windows UI surface")
set(DAWN_BUILD_SAMPLES OFF CACHE INTERNAL "Enables building Dawn's samples")
set(DAWN_BUILD_TESTS OFF CACHE INTERNAL "Enables building Dawn's tests")
set(DAWN_BUILD_PROTOBUF ${cage_use_steam_sockets} CACHE INTERNAL "Build the protobuf dependencies")
set(DAWN_ENABLE_PIC ON CACHE INTERNAL "Build with Position-Independent-Code enabled")
set(DAWN_BUILD_MONOLITHIC_LIBRARY "STATIC" CACHE INTERNAL "Build monolithic library: SHARED, STATIC, or OFF.")

# tint
set(TINT_BUILD_SPV_READER ON CACHE INTERNAL "Build the SPIR-V input reader")
set(TINT_BUILD_WGSL_READER ON CACHE INTERNAL "Build the WGSL input reader")
set(TINT_BUILD_GLSL_WRITER OFF CACHE INTERNAL "Build the GLSL output writer")
set(TINT_BUILD_GLSL_VALIDATOR OFF CACHE INTERNAL "Build the GLSL output validator") # required for glslang
set(TINT_BUILD_HLSL_WRITER OFF CACHE INTERNAL "Build the HLSL output writer")
set(TINT_BUILD_MSL_WRITER ${metal} CACHE INTERNAL "Build the MSL output writer")
set(TINT_BUILD_SPV_WRITER ${vulkan} CACHE INTERNAL "Build the SPIR-V output writer")
set(TINT_BUILD_WGSL_WRITER ON CACHE INTERNAL "Build the WGSL output writer")
set(TINT_ENABLE_INSTALL OFF CACHE INTERNAL "Enable install step for Tint libraries")
set(TINT_BUILD_CMD_TOOLS OFF CACHE INTERNAL "Build the Tint command line tools") # required for glslang
set(TINT_BUILD_IR_BINARY OFF CACHE INTERNAL "Build IR binary format support")
set(TINT_BUILD_TESTS OFF CACHE INTERNAL "Build tests")
set(TINT_ENABLE_IR_VALIDATION ${cage_validate_graphics} CACHE INTERNAL "Enable IR validation for backend codegen")

unset(metal)
unset(vulkan)
unset(linux)


# absl
set(ABSL_PROPAGATE_CXX_STD OFF CACHE INTERNAL "Use CMake C++ standard meta features (e.g. cxx_std_17) that propagate to targets that link to Abseil")
set(ABSL_USE_SYSTEM_INCLUDES ON CACHE INTERNAL "Use CMake C++ standard meta features (e.g. cxx_std_17) that propagate to targets that link to Abseil")


# protobuf
add_library(libprotobuf-mutator INTERFACE) # subvert unnecessary dependencies
set(protobuf_BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build Shared Libraries")
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE INTERNAL "Link static runtime libraries")


# dawn
add_subdirectory(dawn)

# cage-dawn
add_library(cage_dawn INTERFACE)
target_link_libraries(cage_dawn INTERFACE $<TARGET_LINKER_FILE:webgpu_dawn> $<TARGET_LINKER_FILE:webgpu_glfw>)
if(APPLE)
	target_link_libraries(cage_dawn INTERFACE "-framework Cocoa" "-framework IOKit" "-framework Foundation" "-framework IOSurface" "-framework QuartzCore" "-framework Metal")
endif()
target_include_directories(cage_dawn INTERFACE $<TARGET_PROPERTY:webgpu_dawn,INTERFACE_INCLUDE_DIRECTORIES> $<TARGET_PROPERTY:webgpu_glfw,INTERFACE_INCLUDE_DIRECTORIES>)
add_dependencies(cage_dawn webgpu_dawn webgpu_glfw)
add_library(cage::dawn ALIAS cage_dawn)


# spirv
add_library(cage_spirv INTERFACE)
target_link_libraries(cage_spirv INTERFACE $<TARGET_LINKER_FILE:glslang> $<TARGET_LINKER_FILE:glslang-default-resource-limits>)
target_link_libraries(cage_spirv INTERFACE $<TARGET_LINKER_FILE:SPIRV-Tools> $<TARGET_LINKER_FILE:SPIRV-Tools-opt> $<TARGET_LINKER_FILE:SPIRV-Tools-link>)
target_include_directories(cage_spirv INTERFACE "${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/glslang/src")
target_include_directories(cage_spirv INTERFACE "${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/spirv-headers/src/include")
target_include_directories(cage_spirv INTERFACE "${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/spirv-tools/src")
target_include_directories(cage_spirv INTERFACE "${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/spirv-tools/src/include")
target_include_directories(cage_spirv INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/dawn/third_party/spirv-tools")
add_dependencies(cage_spirv glslang glslang-default-resource-limits)
add_dependencies(cage_spirv SPIRV-Tools SPIRV-Tools-opt SPIRV-Tools-link)
add_library(cage::spirv ALIAS cage_spirv)


# protobuf
if(cage_use_steam_sockets)
	set(filename "${CAGE_EXTERNALS_MODULE_PATH}/FindProtobuf.cmake")
	file(WRITE ${filename} "\n")
	macro(append_lib varname libname)
		set(lib $<TARGET_LINKER_FILE:${libname}>)
		set(inc $<TARGET_PROPERTY:${libname},INTERFACE_INCLUDE_DIRECTORIES>)
		cage_gen_module_helper(${filename} ${varname} "${lib}" "${inc}")
	endmacro()
	append_lib(Protobuf protobuf)
	append_lib(Protobuf_LITE protobuf-lite)
	append_lib(Protobuf_PROTOC protoc)
	file(APPEND "${filename}" "set(Protobuf_PROTOC_EXECUTABLE protobuf::protoc)\n")
	configure_file("${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/protobuf/cmake/protobuf-generate.cmake" "${CAGE_EXTERNALS_MODULE_PATH}/dawn/third_party/protobuf/protobuf-generate.cmake" COPYONLY)
	file(APPEND "${filename}" "include(\${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/protobuf/protobuf-generate.cmake)\n")
	configure_file("${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/protobuf/cmake/protobuf-module.cmake.in" "${CAGE_EXTERNALS_MODULE_PATH}/dawn/third_party/protobuf/protobuf-module.cmake" COPYONLY)
	file(APPEND "${filename}" "include(\${CMAKE_CURRENT_LIST_DIR}/dawn/third_party/protobuf/protobuf-module.cmake)\n")
endif()
