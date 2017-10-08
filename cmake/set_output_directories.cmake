
function(set_output_directories where)
	foreach(conf IN ITEMS ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
		string(TOUPPER ${conf} conf_upper)
		string(TOLOWER ${conf} conf_lower)
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${conf_upper} ${where}/${conf_lower} PARENT_SCOPE)
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${conf_upper} ${where}/${conf_lower} PARENT_SCOPE)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${conf_upper} ${where}/${conf_lower} PARENT_SCOPE)
	endforeach(conf)
endfunction(set_output_directories)

