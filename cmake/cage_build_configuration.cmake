
function(cage_build_configuration)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
	set(CMAKE_CXX_STANDARD 11 PARENT_SCOPE)
	if(CMAKE_COMPILER_IS_GNUCXX)
		# gcc: disable warnings about attributes
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes" PARENT_SCOPE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes" PARENT_SCOPE)
	endif()
endfunction(cage_build_configuration)

