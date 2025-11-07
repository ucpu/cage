message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg")

if(CMAKE_C_COMPILER_TARGET MATCHES "x86_64" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CMAKE_ASM_NASM_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/yasm/win64/bin/yasm.exe" CACHE INTERNAL "" FORCE)
		set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -DWIN64 -D__x86_64__ -D\"EXTN(name)=name\"") # target_compile_definitions does not work with nasm
	else()
		set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> -felf64 -DELF -DPIC -D__x86_64__ -o <OBJECT> <SOURCE>" CACHE INTERNAL "") # no <FLAGS>
	endif()
	enable_language(ASM_NASM)
	message(STATUS "CMAKE_ASM_NASM_COMPILER = ${CMAKE_ASM_NASM_COMPILER}")
	set(SIMD_SOURCES x86_64/jsimdcpu.asm x86_64/jfdctflt-sse.asm
		x86_64/jccolor-sse2.asm x86_64/jcgray-sse2.asm x86_64/jchuff-sse2.asm
		x86_64/jcphuff-sse2.asm x86_64/jcsample-sse2.asm x86_64/jdcolor-sse2.asm
		x86_64/jdmerge-sse2.asm x86_64/jdsample-sse2.asm x86_64/jfdctfst-sse2.asm
		x86_64/jfdctint-sse2.asm x86_64/jidctflt-sse2.asm x86_64/jidctfst-sse2.asm
		x86_64/jidctint-sse2.asm x86_64/jidctred-sse2.asm x86_64/jquantf-sse2.asm
		x86_64/jquanti-sse2.asm
		x86_64/jccolor-avx2.asm x86_64/jcgray-avx2.asm x86_64/jcsample-avx2.asm
		x86_64/jdcolor-avx2.asm x86_64/jdmerge-avx2.asm x86_64/jdsample-avx2.asm
		x86_64/jfdctint-avx2.asm x86_64/jidctint-avx2.asm x86_64/jquanti-avx2.asm)
	list(APPEND SIMD_SOURCES x86_64/jsimd.c)
	list(TRANSFORM SIMD_SOURCES PREPEND "jpeg/simd/")
	add_library(jpeg-simd STATIC ${SIMD_SOURCES})
	list(FILTER SIMD_SOURCES INCLUDE REGEX "asm$")
	set_source_files_properties(${SIMD_SOURCES} PROPERTIES LANGUAGE ASM_NASM)
	target_include_directories(jpeg-simd PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/simd" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/simd/x86_64" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/simd/nasm")
	set(WITH_SIMD 1)
endif()

include(CheckIncludeFiles)
set(JPEG_LIB_VERSION 80)
set(VERSION 2.1.92)
set(LIBJPEG_TURBO_VERSION_NUMBER 0)
set(BUILD "customized-for-cage")
set(SIZE_T ${CMAKE_SIZEOF_VOID_P})
set(C_ARITH_CODING_SUPPORTED 1)
set(D_ARITH_CODING_SUPPORTED 1)
set(INLINE "inline")
if(MSVC)
	set(THREAD_LOCAL "__declspec(thread)")
	set(FORCE_INLINE "__forceinline")
else()
	set(THREAD_LOCAL "__thread")
	set(FORCE_INLINE "[[gnu::always_inline]]inline")
endif()
check_include_files("intrin.h" HAVE_INTRIN_H)
configure_file("jpeg/src/jversion.h.in" "jversion.h")
configure_file("jpeg/src/jconfig.h.in" "jconfig.h")
configure_file("jpeg/src/jconfigint.h.in" "jconfigint.h")

set(JPEG16_SOURCES jcapistd.c jccolor.c jcdiffct.c jclossls.c jcmainct.c
	jcprepct.c jcsample.c jdapistd.c jdcolor.c jddiffct.c jdlossls.c jdmainct.c
	jdpostct.c jdsample.c jutils.c)
set(JPEG12_SOURCES ${JPEG16_SOURCES} jccoefct.c jcdctmgr.c jdcoefct.c
	jddctmgr.c jdmerge.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c
	jidctred.c jquant1.c jquant2.c)
set(JPEG_SOURCES ${JPEG12_SOURCES} jcapimin.c jchuff.c jcicc.c jcinit.c
	jclhuff.c jcmarker.c jcmaster.c jcomapi.c jcparam.c jcphuff.c jctrans.c
	jdapimin.c jdatadst.c jdatasrc.c jdhuff.c jdicc.c jdinput.c jdlhuff.c
	jdmarker.c jdmaster.c jdphuff.c jdtrans.c jerror.c jfdctflt.c jmemmgr.c
	jmemnobs.c jpeg_nbits.c)

list(TRANSFORM JPEG16_SOURCES PREPEND "jpeg/src/")
list(TRANSFORM JPEG12_SOURCES PREPEND "jpeg/src/")
list(TRANSFORM JPEG_SOURCES PREPEND "jpeg/src/")

add_library(cage-jpeg12-static OBJECT ${JPEG12_SOURCES})
target_compile_definitions(cage-jpeg12-static PRIVATE "BITS_IN_JSAMPLE=12")
add_library(cage-jpeg16-static OBJECT ${JPEG16_SOURCES})
target_compile_definitions(cage-jpeg16-static PRIVATE "BITS_IN_JSAMPLE=16")
add_library(jpeg-static STATIC ${JPEG_SOURCES}
	"jpeg/src/jaricom.c" "jpeg/src/jcarith.c" "jpeg/src/jdarith.c"
	$<TARGET_OBJECTS:cage-jpeg12-static> $<TARGET_OBJECTS:cage-jpeg16-static>
)
target_include_directories(jpeg-static INTERFACE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/src")
if(TARGET jpeg-simd)
	target_link_libraries(jpeg-static PRIVATE jpeg-simd)
endif()

add_library(cage-jpeg INTERFACE)
target_link_libraries(cage-jpeg INTERFACE jpeg-static)
cage_gen_module(JPEG $<TARGET_LINKER_FILE:cage-jpeg> $<TARGET_PROPERTY:cage-jpeg,INTERFACE_INCLUDE_DIRECTORIES>)
add_library(JPEG::JPEG ALIAS cage-jpeg)
