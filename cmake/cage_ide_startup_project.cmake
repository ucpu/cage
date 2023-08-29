
function(cage_ide_startup_project target)
	set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${target})
endfunction(cage_ide_startup_project)
