
macro(cage_build_configuration)
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
		message(FATAL_ERROR "CMAKE_BUILD_TYPE must be set")
	endif()

	cmake_policy(SET CMP0063 NEW)
	set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

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

	# rpath
	set(CMAKE_MACOSX_RPATH TRUE)
	set(CMAKE_SKIP_RPATH FALSE)
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_SKIP_INSTALL_RPATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
	set(CMAKE_BUILD_RPATH "\$ORIGIN")
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
	set(CMAKE_INSTALL_RPATH "\$ORIGIN")

	if(WIN32)
		# enable UTF-8
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

		# conformance to standard
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /permissive-")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")

		# whole program optimizations
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GL")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
		set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

		# compatibility hack
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")

		# disable some warnings
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS /D_ENABLE_EXTENDED_ALIGNED_STORAGE")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd26812") # The enum type ___ is unscoped. Prefer 'enum class' over 'enum'
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd26451") # Arithmetic overflow: Using operator ___ on a 4 byte value and then casting the result to a 8 byte value.

		# optionally improve runtime performance in debug builds (basic inlining)
		option(cage_faster_debug "enable some optimizations to improve performance in debug builds" ON)
		if(cage_faster_debug)
			string(REGEX REPLACE "/Ob[0-9]" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
			string(REGEX REPLACE "/Ob[0-9]" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
			set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Ob1")
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Ob1 /D_ITERATOR_DEBUG_LEVEL=0")
		endif()

		# optionally improve runtime performance in release builds (more aggressive inlining)
		option(cage_faster_release "enable more aggressive inlining in release builds" OFF)
		if(cage_faster_release)
			string(REGEX REPLACE "/Ob[0-9]" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
			string(REGEX REPLACE "/Ob[0-9]" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ob3")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ob3")
		endif()

		# 8 MB default stack size
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8388608")

		# multi-process compilation
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	else()
		# make runtime loader look for local symbols first
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Bsymbolic")

		# link time optimizations
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")

		# disable some warnings
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes -Wno-abi")
	endif()

	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		# prevent requiring executable stack
		add_link_options(-Wl,-z,noexecstack)
	endif()

	if(APPLE)
		# disable more warnings
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
	endif()

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		# __builtin_assume has side effects that are discarded
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-assume")
	endif()
endmacro(cage_build_configuration)
