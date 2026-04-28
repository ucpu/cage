message(STATUS "----------------------------------------------------------------")
message(STATUS "jpeg")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg")

if(CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "x64" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
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
	list(APPEND SIMD_SOURCES jsimd.c)
	list(TRANSFORM SIMD_SOURCES PREPEND "jpeg/simd/")
	add_library(jpeg-simd STATIC ${SIMD_SOURCES})
	list(FILTER SIMD_SOURCES INCLUDE REGEX "asm$")
	set_source_files_properties(${SIMD_SOURCES} PROPERTIES LANGUAGE ASM_NASM)
	target_include_directories(jpeg-simd PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/simd" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/simd/x86_64" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/simd/nasm")
	set(WITH_SIMD 1)
	set(SIMD_ARCHITECTURE 0)
else()
	set(SIMD_ARCHITECTURE -1)
endif()

include(CheckIncludeFiles)
check_include_files("intrin.h" HAVE_INTRIN_H)
set(JPEG_LIB_VERSION 80)
set(VERSION 3.1.91)
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
configure_file("jpeg/src/jversion.h.in" "jversion.h")
configure_file("jpeg/src/jconfig.h.in" "jconfig.h")
configure_file("jpeg/src/jconfigint.h.in" "jconfigint.h")

set(JPEG_SOURCES src/jcapimin.c
	src/wrapper/jcapistd-8.c src/wrapper/jcapistd-12.c src/wrapper/jcapistd-16.c
	src/wrapper/jccoefct-8.c src/wrapper/jccoefct-12.c
	src/wrapper/jccolor-8.c src/wrapper/jccolor-12.c src/wrapper/jccolor-16.c
	src/wrapper/jcdctmgr-8.c src/wrapper/jcdctmgr-12.c
	src/wrapper/jcdiffct-8.c src/wrapper/jcdiffct-12.c src/wrapper/jcdiffct-16.c
	src/jchuff.c src/jcicc.c src/jcinit.c src/jclhuff.c
	src/wrapper/jclossls-8.c src/wrapper/jclossls-12.c src/wrapper/jclossls-16.c
	src/wrapper/jcmainct-8.c src/wrapper/jcmainct-12.c src/wrapper/jcmainct-16.c
	src/jcmarker.c src/jcmaster.c src/jcomapi.c src/jcparam.c src/jcphuff.c
	src/wrapper/jcprepct-8.c src/wrapper/jcprepct-12.c src/wrapper/jcprepct-16.c
	src/wrapper/jcsample-8.c src/wrapper/jcsample-12.c src/wrapper/jcsample-16.c
	src/jctrans.c src/jdapimin.c
	src/wrapper/jdapistd-8.c src/wrapper/jdapistd-12.c src/wrapper/jdapistd-16.c
	src/jdatadst.c src/jdatasrc.c
	src/wrapper/jdcoefct-8.c src/wrapper/jdcoefct-12.c
	src/wrapper/jdcolor-8.c src/wrapper/jdcolor-12.c src/wrapper/jdcolor-16.c
	src/wrapper/jddctmgr-8.c src/wrapper/jddctmgr-12.c
	src/wrapper/jddiffct-8.c src/wrapper/jddiffct-12.c src/wrapper/jddiffct-16.c
	src/jdhuff.c src/jdicc.c src/jdinput.c src/jdlhuff.c
	src/wrapper/jdlossls-8.c src/wrapper/jdlossls-12.c src/wrapper/jdlossls-16.c
	src/wrapper/jdmainct-8.c src/wrapper/jdmainct-12.c src/wrapper/jdmainct-16.c
	src/jdmarker.c src/jdmaster.c
	src/wrapper/jdmerge-8.c src/wrapper/jdmerge-12.c
	src/jdphuff.c
	src/wrapper/jdpostct-8.c src/wrapper/jdpostct-12.c src/wrapper/jdpostct-16.c
	src/wrapper/jdsample-8.c src/wrapper/jdsample-12.c src/wrapper/jdsample-16.c
	src/jdtrans.c src/jerror.c src/jfdctflt.c
	src/wrapper/jfdctfst-8.c src/wrapper/jfdctfst-12.c
	src/wrapper/jfdctint-8.c src/wrapper/jfdctint-12.c
	src/wrapper/jidctflt-8.c src/wrapper/jidctflt-12.c
	src/wrapper/jidctfst-8.c src/wrapper/jidctfst-12.c
	src/wrapper/jidctint-8.c src/wrapper/jidctint-12.c
	src/wrapper/jidctred-8.c src/wrapper/jidctred-12.c
	src/jmemmgr.c src/jmemnobs.c src/jpeg_nbits.c
	src/wrapper/jquant1-8.c src/wrapper/jquant1-12.c
	src/wrapper/jquant2-8.c src/wrapper/jquant2-12.c
	src/wrapper/jutils-8.c src/wrapper/jutils-12.c src/wrapper/jutils-16.c)

list(TRANSFORM JPEG_SOURCES PREPEND "jpeg/")

add_library(jpeg-static STATIC ${JPEG_SOURCES}
	"jpeg/src/jaricom.c" "jpeg/src/jcarith.c" "jpeg/src/jdarith.c"
)
target_include_directories(jpeg-static INTERFACE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/jpeg/src")
if(TARGET jpeg-simd)
	target_link_libraries(jpeg-static PRIVATE jpeg-simd)
endif()

add_library(cage-jpeg INTERFACE)
target_link_libraries(cage-jpeg INTERFACE jpeg-static)
cage_gen_module(JPEG $<TARGET_LINKER_FILE:cage-jpeg> $<TARGET_PROPERTY:cage-jpeg,INTERFACE_INCLUDE_DIRECTORIES>)
add_library(JPEG::JPEG ALIAS cage-jpeg)
