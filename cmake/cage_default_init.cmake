
macro(cage_default_init)
	cage_disable_source_build()
	cage_build_configuration()
	cage_build_destination()
	cage_assets_init()
	cage_copy_redist()
endmacro(cage_default_init)
