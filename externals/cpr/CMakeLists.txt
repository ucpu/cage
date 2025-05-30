message(STATUS "----------------------------------------------------------------")
message(STATUS "cpr")
message(STATUS "----------------------------------------------------------------")

set(CPR_USE_SYSTEM_CURL ON CACHE INTERNAL "")

if (NOT WIN32)
	set(CPR_FORCE_MBEDTLS_BACKEND ON CACHE INTERNAL "")
endif()

add_subdirectory(cpr)
