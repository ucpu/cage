message(STATUS "----------------------------------------------------------------")
message(STATUS "mbedtls")
message(STATUS "----------------------------------------------------------------")

set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON CACHE INTERNAL "")
set(ENABLE_PROGRAMS OFF CACHE INTERNAL "")
set(ENABLE_TESTING OFF CACHE INTERNAL "")
set(GEN_FILES OFF CACHE INTERNAL "")
set(MBEDTLS_AS_SUBPROJECT ON CACHE INTERNAL "")
set(MBEDTLS_FATAL_WARNINGS OFF CACHE INTERNAL "")

set(CMAKE_DISABLE_SOURCE_CHANGES OFF)
add_subdirectory(mbedtls)

cage_gen_module(MbedTLS "$<TARGET_LINKER_FILE:mbedtls>;$<TARGET_LINKER_FILE:mbedx509>;$<TARGET_LINKER_FILE:mbedcrypto>" "${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/include")
set(filename "${CAGE_EXTERNALS_MODULE_PATH}/FindMBEDTLS.cmake")
cage_gen_module_helper(${filename} MBEDX509 $<TARGET_LINKER_FILE:mbedx509> "")
cage_gen_module_helper(${filename} MBEDCRYPTO $<TARGET_LINKER_FILE:mbedcrypto> "")
