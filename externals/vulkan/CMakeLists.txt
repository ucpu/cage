message(STATUS "----------------------------------------------------------------")
message(STATUS "vulkan")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE INTERNAL "VULKAN_HEADERS_ENABLE_TESTS")
set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE INTERNAL "VULKAN_HEADERS_ENABLE_INSTALL")

set(VULKAN_HPP_BUILD_CXX_MODULE OFF CACHE INTERNAL "VULKAN_HPP_BUILD_CXX_MODULE")
set(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC OFF CACHE INTERNAL "VULKAN_HPP_DISPATCH_LOADER_DYNAMIC")

set(VOLK_NAMESPACE ON CACHE INTERNAL "VOLK_NAMESPACE")
set(VOLK_PULL_IN_VULKAN OFF CACHE INTERNAL "VOLK_PULL_IN_VULKAN")

add_subdirectory(headers)
cage_gen_module(Vulkan "Vulkan::Headers" "${CMAKE_CURRENT_SOURCE_DIR}/headers/include")
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan OFF)
add_subdirectory(hpp)
add_subdirectory(volk)
add_subdirectory(vma)
add_subdirectory(bootstrap)

add_library(cage_vulkan_public INTERFACE)
add_library(cage_vulkan_private INTERFACE)
target_link_libraries(cage_vulkan_public INTERFACE Vulkan::Headers Vulkan::Hpp GPUOpen::VulkanMemoryAllocator)
target_link_libraries(cage_vulkan_private INTERFACE volk::volk vk-bootstrap::vk-bootstrap)
add_library(cage::vulkan_public ALIAS cage_vulkan_public)
add_library(cage::vulkan_private ALIAS cage_vulkan_private)
