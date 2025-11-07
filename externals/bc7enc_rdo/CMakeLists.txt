message(STATUS "----------------------------------------------------------------")
message(STATUS "bc7enc_rdo")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

add_library(bc7enc_rdo STATIC
	bc7enc_rdo/bc7decomp.cpp bc7enc_rdo/bc7decomp.h
	bc7enc_rdo/bc7enc.cpp bc7enc_rdo/bc7enc.h
	bc7enc_rdo/rgbcx.cpp bc7enc_rdo/rgbcx.h
)

target_include_directories(bc7enc_rdo PRIVATE bc7enc_rdo)
target_include_directories(bc7enc_rdo INTERFACE .)
