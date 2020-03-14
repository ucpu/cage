
set(cage_output_redist_source "${CMAKE_CURRENT_LIST_DIR}")

function(cage_output_redist where)
	configure_file("${cage_output_redist_source}/../externals/assimp/assimp/LICENSE" "${where}/redist/licenses/assimp" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/dr_libs/dr_libs/README.md" "${where}/redist/licenses/dr_libs" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/fastnoise/fastnoise/LICENSE" "${where}/redist/licenses/fastnoise" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/freetype/freetype/docs/LICENSE.TXT" "${where}/redist/licenses/freetype" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/freetype/freetype/docs/FTL.TXT" "${where}/redist/licenses/freetype_ftl" COPYONLY)
	#configure_file("${cage_output_redist_source}/../externals/freetype/freetype/docs/GPLv2.TXT" "${where}/redist/licenses/freetype_gpl" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/glfw/glfw/LICENSE.md" "${where}/redist/licenses/glfw" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/glm/glm/manual.md" "${where}/redist/licenses/glm" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/jpeg/jpeg/LICENSE.md" "${where}/redist/licenses/jpeg" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/libpng/libpng/LICENSE" "${where}/redist/licenses/png" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/libsoundio/libsoundio/LICENSE" "${where}/redist/licenses/soundio" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/libtiff/libtiff/COPYRIGHT" "${where}/redist/licenses/tiff" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/libzip/libzip/LICENSE" "${where}/redist/licenses/zip" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/msdfgen/msdfgen/LICENSE.txt" "${where}/redist/licenses/msdfgen" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/ogg/ogg/COPYING" "${where}/redist/licenses/ogg" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/optick/optick/LICENSE" "${where}/redist/licenses/optick" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/simplefilewatcher/simplefilewatcher/License.txt" "${where}/redist/licenses/simplefilewatcher" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/utfcpp/utfcpp/LICENSE" "${where}/redist/licenses/utfcpp" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/vorbis/vorbis/COPYING" "${where}/redist/licenses/vorbis" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/xsimd/xsimd/LICENSE" "${where}/redist/licenses/xsimd" COPYONLY)
	configure_file("${cage_output_redist_source}/../externals/zlib-ng/zlib-ng/LICENSE.md" "${where}/redist/licenses/zlib-ng" COPYONLY)
	configure_file("${cage_output_redist_source}/../LICENSE" "${where}/redist/licenses/cage" COPYONLY)
endfunction(cage_output_redist)

