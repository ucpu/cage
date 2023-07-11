cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "protobuf")
message(STATUS "----------------------------------------------------------------")

set(protobuf_INSTALL OFF CACHE INTERNAL "protobuf install")
set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "protobuf tests")
set(protobuf_BUILD_PROTOBUF_BINARIES ON CACHE INTERNAL "protobuf protobuf binaries")
set(use_precompiled_protoc ${WIN32})
if(use_precompiled_protoc)
	message(STATUS "using precompiled protoc")
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(Protobuf_PROTOC_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/protoc/win64/bin/protoc.exe")
	else()
		set(Protobuf_PROTOC_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/protoc/win32/bin/protoc.exe")
	endif()
	set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE INTERNAL "protobuf protoc binaries")
else()
	message(STATUS "compiling protoc from source")
	set(protobuf_BUILD_PROTOC_BINARIES ON CACHE INTERNAL "protobuf protoc binaries")
endif()
set(protobuf_BUILD_SHARED_LIBS OFF CACHE INTERNAL "protobuf shared lib")
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE INTERNAL "protobuf msvc static runtime")
set(utf8_range_ENABLE_TESTS OFF CACHE INTERNAL "protobuf utf8_range tests")
set(utf8_range_ENABLE_INSTALL OFF CACHE INTERNAL "protobuf utf8_range install")
set(ABSL_PROPAGATE_CXX_STD ON CACHE INTERNAL "protobuf absl propagate cxx standard")
set(ABSL_IDE_FOLDER "cage/externals/abseil")

add_subdirectory(protobuf)

set(filename "${CAGE_EXTERNALS_MODULE_PATH}/FindProtobuf.cmake")
file(WRITE ${filename} "\n")
macro(append_lib varname libname)
	set(lib $<TARGET_LINKER_FILE:${libname}>)
	set(inc $<TARGET_PROPERTY:${libname},INTERFACE_INCLUDE_DIRECTORIES>)
	cage_gen_module_helper(${filename} ${varname} "${lib}" "${inc}")
endmacro()
append_lib(Protobuf protobuf)
append_lib(Protobuf_LITE protobuf-lite)
if(use_precompiled_protoc)
	add_executable(protoc IMPORTED GLOBAL)
	set_target_properties(protoc PROPERTIES IMPORTED_LOCATION "${Protobuf_PROTOC_EXECUTABLE}")
	add_executable(protobuf::protoc ALIAS protoc)
	file(APPEND "${filename}" "set(Protobuf_PROTOC_EXECUTABLE \"${Protobuf_PROTOC_EXECUTABLE}\")\n")
else()
	append_lib(Protobuf_PROTOC protoc)
	file(APPEND "${filename}" "set(Protobuf_PROTOC_EXECUTABLE protobuf::protoc)\n")
endif()
configure_file("${CMAKE_CURRENT_LIST_DIR}/protobuf/cmake/protobuf-generate.cmake" "${CAGE_EXTERNALS_MODULE_PATH}/protobuf/protobuf-generate.cmake" COPYONLY)
file(APPEND "${filename}" "include(\${CMAKE_CURRENT_LIST_DIR}/protobuf/protobuf-generate.cmake)\n")
configure_file("${CMAKE_CURRENT_LIST_DIR}/protobuf/cmake/protobuf-module.cmake.in" "${CAGE_EXTERNALS_MODULE_PATH}/protobuf/protobuf-module.cmake" COPYONLY)
file(APPEND "${filename}" "include(\${CMAKE_CURRENT_LIST_DIR}/protobuf/protobuf-module.cmake)\n")

set(code_after [[
#ifdef PROTOBUF_PORT_
#include "google/protobuf/port_undef.inc"
#endif
]])
set_target_properties(libprotobuf libprotobuf-lite PROPERTIES
	UNITY_BUILD ON
	UNITY_BUILD_BATCH_SIZE 100
	UNITY_BUILD_CODE_AFTER_INCLUDE "${code_after}")
file(GLOB_RECURSE pb_files
	"protobuf/src/google/protobuf/*.pb.cc"
	"protobuf/src/google/protobuf/extension_set_heavy.cc"
	"protobuf/src/google/protobuf/feature_resolver.cc"
	"protobuf/src/google/protobuf/generated_message_reflection.cc"
	"protobuf/src/google/protobuf/strtod.cc"
	"protobuf/src/google/protobuf/untyped_message.cc"
	"protobuf/src/google/protobuf/zero_copy_sink.cc"
	"protobuf/src/google/protobuf/implicit_weak_message.cc"
)
set_source_files_properties(${pb_files} TARGET_DIRECTORY libprotobuf PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

