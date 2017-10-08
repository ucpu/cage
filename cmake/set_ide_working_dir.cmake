
function(set_ide_working_dir target directory)
	if(${CMAKE_VERSION} VERSION_GREATER 3.8)
		set_target_properties(${target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${directory})
	endif()
endfunction(set_ide_working_dir)

function(set_ide_working_dir_in_place target)
	set_ide_working_dir(${target} "$(OutDir)")
endfunction(set_ide_working_dir_in_place)
