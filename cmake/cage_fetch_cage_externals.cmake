
if(NOT DEFINED egac_fetch_cage_externals_done) # INTERNAL implies FORCE
	set(egac_fetch_cage_externals_done OFF CACHE INTERNAL "")
endif()

function(cage_fetch_cage_externals)
	if(egac_fetch_cage_externals_done)
		return()
	endif()
	execute_process(COMMAND "bash" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../submodules.sh" WORKING_DIRECTORY "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." COMMAND_ECHO STDOUT COMMAND_ERROR_IS_FATAL ANY)
	set(egac_fetch_cage_externals_done ON CACHE INTERNAL "" FORCE)
endfunction(cage_fetch_cage_externals)

