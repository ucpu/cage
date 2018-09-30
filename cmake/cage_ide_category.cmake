
function(cage_ide_category target category)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set_target_properties(${target} PROPERTIES FOLDER ${category})
endfunction(cage_ide_groups)

