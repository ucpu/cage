
macro(cage_build_configuration)
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
		message(FATAL_ERROR "CMAKE_BUILD_TYPE needs to be set")
	endif()

	cmake_policy(SET CMP0063 NEW)

	# language standard
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	set(CMAKE_C_STANDARD 17)
	set(CMAKE_C_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	# default visibility hidden
	set(CMAKE_C_VISIBILITY_PRESET hidden)
	set(CMAKE_CXX_VISIBILITY_PRESET hidden)
	set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

	if(MSVC)
		# multi-process compilation
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

		# conformance to standard
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /permissive-")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")

		# whole program optimizations
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GL")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
		set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

		# disable compiler warnings:
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_ENABLE_EXTENDED_ALIGNED_STORAGE")

		# optionally improve runtime performance in debug builds
		option(CAGE_FASTER_DEBUG "enable some optimizations to improve performance in debug builds" ON)
		if(CAGE_FASTER_DEBUG)
			set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Ob1")
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Ob1 /D_ITERATOR_DEBUG_LEVEL=0")
		endif()
	else()
		# link time optimizations
		# fails with clang - generates empty archive in zlib-ng
		if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
		endif()

		# disable warnings about attributes
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")

		# no warnings about changes in ABI
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-abi")
	endif()
endmacro(cage_build_configuration)

