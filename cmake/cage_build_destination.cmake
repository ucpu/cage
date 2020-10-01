
set(cage_build_destination_directory "${CMAKE_BINARY_DIR}/result" CACHE PATH "Directory to put in all final build products (subdir per configuration).")
set(cage_build_destination_archives OFF CACHE BOOL "Move archive artifacts into the output directory.")

function(cage_build_destination)
	foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
		string(TOUPPER ${conf} conf_upper)
		string(TOLOWER ${conf} conf_lower)
		if(${cage_build_destination_archives})
			set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${conf_upper} "${cage_build_destination_directory}/${conf_lower}/lib" PARENT_SCOPE)
		endif()
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${conf_upper} "${cage_build_destination_directory}/${conf_lower}" PARENT_SCOPE)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf_upper} "${cage_build_destination_directory}/${conf_lower}" PARENT_SCOPE)
	endforeach(conf)
endfunction(cage_build_destination)

