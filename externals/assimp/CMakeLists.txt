cmake_minimum_required(VERSION 3.10)

message(STATUS "----------------------------------------------------------------")
message(STATUS "assimp")
message(STATUS "----------------------------------------------------------------")

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "BUILD_SHARED_LIBS")
option(ASSIMP_NO_EXPORT "Disable Assimp's export functionality." ON)
option(ASSIMP_BUILD_ASSIMP_TOOLS "If the supplementary tools for Assimp are built in addition to the library." OFF)
option(ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)

# disable some formats to speed up build
# one may enable them in the cache or by setting the default before including this cmake
foreach(name AMF AC ASE ASSBIN ASSXML B3D BVH DXF CSM HMP IRRMESH IRR LWO LWS MD2 MD3 MD5 MDC MDL NFF NDO OFF OGRE OPENGEX MS3D COB IFC XGL Q3D Q3BSP RAW SIB SMD STL TERRAGEN 3D X3D GLTF 3MF MMD STEP)
	option(ASSIMP_BUILD_${name}_IMPORTER "Build ${name} importer" OFF)
endforeach()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

add_subdirectory(assimp)

# assimp/contrib/unzip/unzip.c somehow incorrectly detects zlib(-ng) version and tries to use undefined type
target_compile_definitions(assimp PRIVATE NOUNCRYPT=1 ZEXPORT=)

target_include_directories(assimp INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include")
target_include_directories(assimp INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/assimp/include")

