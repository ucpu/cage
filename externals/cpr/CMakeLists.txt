cmake_minimum_required(VERSION 3.22)

message(STATUS "----------------------------------------------------------------")
message(STATUS "cpr")
message(STATUS "----------------------------------------------------------------")

set(CPR_CURL_NOSIGNAL ON CACHE INTERNAL "")
set(CPR_USE_SYSTEM_CURL OFF CACHE INTERNAL "")
set(CURL_ZLIB OFF CACHE INTERNAL "")

add_subdirectory(cpr)
