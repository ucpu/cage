message(STATUS "----------------------------------------------------------------")
message(STATUS "mbedtls")
message(STATUS "----------------------------------------------------------------")

set(ENABLE_PROGRAMS OFF CACHE INTERNAL "")
set(ENABLE_TESTING OFF CACHE INTERNAL "")

add_subdirectory(mbedtls)

cage_gen_module(MbedTLS "$<TARGET_LINKER_FILE:mbedtls>;$<TARGET_LINKER_FILE:mbedx509>;$<TARGET_LINKER_FILE:mbedcrypto>" "${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/include")
set(filename "${CAGE_EXTERNALS_MODULE_PATH}/FindMBEDTLS.cmake")
cage_gen_module_helper(${filename} MBEDX509 $<TARGET_LINKER_FILE:mbedx509> "")
cage_gen_module_helper(${filename} MBEDCRYPTO $<TARGET_LINKER_FILE:mbedcrypto> "")
