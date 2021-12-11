cmake_minimum_required(VERSION 3.16)

message(STATUS "----------------------------------------------------------------")
message(STATUS "basis_universal")
message(STATUS "----------------------------------------------------------------")

add_library(basisu STATIC
	basis_universal/encoder/apg_bmp.c
	basis_universal/encoder/apg_bmp.h
	basis_universal/encoder/basisu_astc_decomp.cpp
	basis_universal/encoder/basisu_astc_decomp.h
	basis_universal/encoder/basisu_backend.cpp
	basis_universal/encoder/basisu_backend.h
	basis_universal/encoder/basisu_basis_file.cpp
	basis_universal/encoder/basisu_basis_file.h
	basis_universal/encoder/basisu_bc7enc.cpp
	basis_universal/encoder/basisu_bc7enc.h
	basis_universal/encoder/basisu_comp.cpp
	basis_universal/encoder/basisu_comp.h
	basis_universal/encoder/basisu_enc.cpp
	basis_universal/encoder/basisu_enc.h
	basis_universal/encoder/basisu_etc.cpp
	basis_universal/encoder/basisu_etc.h
	basis_universal/encoder/basisu_frontend.cpp
	basis_universal/encoder/basisu_frontend.h
	basis_universal/encoder/basisu_global_selector_palette_helpers.cpp
	basis_universal/encoder/basisu_global_selector_palette_helpers.h
	basis_universal/encoder/basisu_gpu_texture.cpp
	basis_universal/encoder/basisu_gpu_texture.h
	basis_universal/encoder/basisu_kernels_declares.h
	basis_universal/encoder/basisu_kernels_imp.h
	basis_universal/encoder/basisu_kernels_sse.cpp
	basis_universal/encoder/basisu_miniz.h
	basis_universal/encoder/basisu_pvrtc1_4.cpp
	basis_universal/encoder/basisu_pvrtc1_4.h
	basis_universal/encoder/basisu_resample_filters.cpp
	basis_universal/encoder/basisu_resampler.cpp
	basis_universal/encoder/basisu_resampler.h
	basis_universal/encoder/basisu_resampler_filters.h
	basis_universal/encoder/basisu_ssim.cpp
	basis_universal/encoder/basisu_ssim.h
	basis_universal/encoder/basisu_uastc_enc.cpp
	basis_universal/encoder/basisu_uastc_enc.h
	basis_universal/encoder/cppspmd_flow.h
	basis_universal/encoder/cppspmd_math.h
	basis_universal/encoder/cppspmd_math_declares.h
	basis_universal/encoder/cppspmd_sse.h
	basis_universal/encoder/cppspmd_type_aliases.h
	basis_universal/encoder/jpgd.cpp
	basis_universal/encoder/jpgd.h
	basis_universal/encoder/lodepng.cpp
	basis_universal/encoder/lodepng.h

	basis_universal/transcoder/basisu.h
	basis_universal/transcoder/basisu_containers.h
	basis_universal/transcoder/basisu_containers_impl.h
	basis_universal/transcoder/basisu_file_headers.h
	basis_universal/transcoder/basisu_global_selector_cb.h
	basis_universal/transcoder/basisu_global_selector_palette.h
	basis_universal/transcoder/basisu_transcoder.cpp
	basis_universal/transcoder/basisu_transcoder.h
	basis_universal/transcoder/basisu_transcoder_internal.h
	basis_universal/transcoder/basisu_transcoder_uastc.h
)

target_include_directories(basisu INTERFACE ".")
target_compile_definitions(basisu PRIVATE BASISU_SUPPORT_SSE=1 BASISD_SUPPORT_KTX2_ZSTD=1)
target_compile_definitions(basisu PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL)
if (NOT MSVC)
	target_compile_options(basisu PRIVATE -msse4.1)
endif()
