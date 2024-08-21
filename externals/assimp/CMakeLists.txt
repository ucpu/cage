cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "assimp")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
set(ASSIMP_INSTALL OFF CACHE INTERNAL "Disable this if you want to use assimp as a submodule.")
set(ASSIMP_BUILD_ZLIB OFF CACHE INTERNAL "Build your own zlib")
set(ASSIMP_NO_EXPORT ON CACHE INTERNAL "Disable Assimp's export functionality.")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "If the supplementary tools for Assimp are built in addition to the library.")
set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "If the test suite for Assimp is built in addition to the library.")

# disable some formats to speed up build
# one may enable them in the cache or by setting the default before including this cmake
foreach(name AMF AC ASE ASSBIN ASSXML B3D BVH DXF CSM HMP IRRMESH IRR LWO LWS MD2 MD3 MD5 MDC MDL NFF NDO OFF OGRE OPENGEX MS3D COB IFC XGL Q3D Q3BSP RAW SIB SMD STL TERRAGEN 3D X3D 3MF MMD STEP)
	option(ASSIMP_BUILD_${name}_IMPORTER "Build ${name} importer" OFF)
endforeach()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

add_subdirectory(assimp)

# assimp/contrib/unzip somehow incorrectly detects zlib(-ng) version and tries to use undefined type
target_compile_definitions(assimp PRIVATE "ZEXPORT=" "z_crc_t=uint32_t")

target_precompile_headers(assimp PRIVATE
	<limits.h>
	<math.h>
	<stddef.h>
	<stdint.h>
	<string.h>
	<sys/types.h>
	"$<$<COMPILE_LANGUAGE:CXX>:<algorithm$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cctype$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<chrono$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cmath$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cstdarg$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cstddef$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cstdint$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cstdio$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cstdlib$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<cstring$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<exception$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<functional$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<iomanip$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<limits$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<list$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<locale$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<map$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<memory$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<new$<ANGLE-R>>" 
	"$<$<COMPILE_LANGUAGE:CXX>:<numeric$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<set$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<sstream$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<stdexcept$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<string$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<unordered_map$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<unordered_set$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<utility$<ANGLE-R>>"
	"$<$<COMPILE_LANGUAGE:CXX>:<vector$<ANGLE-R>>"
)
