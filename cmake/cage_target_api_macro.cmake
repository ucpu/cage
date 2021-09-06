
macro(cage_target_api_macro target macroname)
if(MSVC)
	target_compile_definitions(${target} PRIVATE "${macroname}=__declspec(dllexport)")
	target_compile_definitions(${target} INTERFACE "${macroname}=__declspec(dllimport)")
else()
	target_compile_definitions(${target} PUBLIC "${macroname}=[[gnu::visibility(\"default\")]]")
endif()
endmacro(cage_target_api_macro)

