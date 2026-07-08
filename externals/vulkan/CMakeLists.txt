message(STATUS "----------------------------------------------------------------")
message(STATUS "vulkan")
message(STATUS "----------------------------------------------------------------")

cage_heavily_optimized()

set(VULKAN_HEADERS_ENABLE_TESTS OFF CACHE INTERNAL "VULKAN_HEADERS_ENABLE_TESTS")
set(VULKAN_HEADERS_ENABLE_INSTALL OFF CACHE INTERNAL "VULKAN_HEADERS_ENABLE_INSTALL")

set(VULKAN_HPP_BUILD_CXX_MODULE OFF CACHE INTERNAL "VULKAN_HPP_BUILD_CXX_MODULE")
set(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC OFF CACHE INTERNAL "VULKAN_HPP_DISPATCH_LOADER_DYNAMIC")

set(VOLK_NAMESPACE ON CACHE INTERNAL "VOLK_NAMESPACE")
set(VOLK_PULL_IN_VULKAN ON CACHE INTERNAL "VOLK_PULL_IN_VULKAN")

set(VK_BOOTSTRAP_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "VK_BOOTSTRAP_POSITION_INDEPENDENT_CODE")

add_subdirectory(headers)
cage_gen_module(Vulkan "Vulkan::Headers" "${CMAKE_CURRENT_SOURCE_DIR}/headers/include")
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan OFF)
add_subdirectory(hpp)
add_subdirectory(volk)
add_subdirectory(vma)
add_subdirectory(bootstrap)

add_library(cage_vulkan INTERFACE)
target_link_libraries(cage_vulkan INTERFACE Vulkan::Headers Vulkan::Hpp GPUOpen::VulkanMemoryAllocator volk::volk vk-bootstrap::vk-bootstrap)
add_library(cage::vulkan ALIAS cage_vulkan)
