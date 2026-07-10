#include <cstring>

#define GLFW_INCLUDE_VULKAN 1
#include <GLFW/glfw3.h>

#include "../window/private.h"
#include "gpu.h"

#include <cage-core/debug.h>
#include <cage-engine/window.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace cage
{
	namespace
	{
		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
		{
			const char *msg = pCallbackData->pMessage;
			uint32 len = std::strlen(msg);
			len = min(len, String::MaxLength / 2);

			SeverityEnum sev = SeverityEnum::Info;
			if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
				sev = SeverityEnum::Hint;
			if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				sev = SeverityEnum::Warning;
			if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				sev = SeverityEnum::Error;

			CAGE_LOG(sev, "vulkan", String(PointerRange(msg, msg + len)));

			if (sev >= SeverityEnum::Error)
			{
				detail::debugBreakpoint();
			}

			return VK_FALSE;
		}

		void logTruncate(SeverityEnum severity, const std::string &str)
		{
			uint32 len = str.length();
			len = min(len, String::MaxLength / 2);
			CAGE_LOG(severity, "vulkan", String(PointerRange(str.data(), str.data() + len)));
		}

		void logError(const vkb::Error &err)
		{
			logTruncate(SeverityEnum::Note, err.type.message());
			for (const auto &it : err.detailed_failure_reasons)
				logTruncate(SeverityEnum::Note, it);
			CAGE_LOG_THROW(Stringizer() + "error code: " + err.type.value());
		}

		template<class T>
		T &&handleResult(vkb::Result<T> &&r)
		{
			if (!r.has_value())
			{
				logError(r.full_error());
				CAGE_THROW_ERROR(Exception, "error in vulkan bootstrap");
			}
			return std::move(r.value());
		}
	}

	namespace gpuImpl
	{
		WindowGpuContextData::WindowGpuContextData(vk::Instance instance_) : instance(instance_) {}

		WindowGpuContextData::~WindowGpuContextData() {}

		void WindowGpuContextData::init()
		{
			// todo
			// create semaphores and fences, if needed
			// update images and image views
		}

		void WindowGpuContextData::clear()
		{
			instance.destroySurfaceKHR(surface);
			surface = nullptr;
			// todo
			// destroy semaphores and fences
		}

		Device::Bootstrap::~Bootstrap()
		{
			vkb::destroy_device(dev);
			vkb::destroy_instance(inst);
		}

		Device::Device(const gpu::GpuDeviceDescriptor &desc)
		{
			CAGE_LOG(SeverityEnum::Info, "gpu", "creating gpu device");

			{
				bootstrapInit(desc);
				instance = bootstrap.inst.instance;
				physicalDevice = bootstrap.phys.physical_device;
				device = bootstrap.dev.device;
				queueTransfer = bootstrap.qt;
				queueGraphics = bootstrap.qg;
				queuePresent = bootstrap.qp;
			}

			{
				VULKAN_HPP_DEFAULT_DISPATCHER.init(instance, device);
			}

			{
				VmaVulkanFunctions funcs = {};
				funcs.vkGetInstanceProcAddr = bootstrap.inst.fp_vkGetInstanceProcAddr;
				funcs.vkGetDeviceProcAddr = bootstrap.inst.fp_vkGetDeviceProcAddr;
				VmaAllocatorCreateInfo info = {};
				info.pVulkanFunctions = &funcs;
				info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
				info.instance = (VkInstance)instance;
				info.physicalDevice = (VkPhysicalDevice)physicalDevice;
				info.device = (VkDevice)device;
				info.vulkanApiVersion = VK_API_VERSION_1_3;
				check("vmaCreateAllocator", vmaCreateAllocator(&info, &allocator));
			}

			{
				const vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
				CAGE_LOG(SeverityEnum::Info, "gpu", Stringizer() + "gpu device name: " + props.deviceName);
				CAGE_LOG(SeverityEnum::Info, "gpu", Stringizer() + "gpu device type: " + vk::to_string(props.deviceType).c_str());
			}
			{
				vk::PhysicalDeviceDriverProperties driverProps;
				vk::PhysicalDeviceProperties2 props2;
				props2.pNext = &driverProps;
				physicalDevice.getProperties2(&props2);
				CAGE_LOG(SeverityEnum::Info, "gpu", Stringizer() + "gpu driver name: " + driverProps.driverName);
				CAGE_LOG(SeverityEnum::Info, "gpu", Stringizer() + "gpu driver info: " + driverProps.driverInfo);
			}
			{
				const vk::PhysicalDeviceMemoryProperties mem = physicalDevice.getMemoryProperties();
				for (uint32 i = 0; i < mem.memoryHeapCount; i++)
				{
					CAGE_LOG(SeverityEnum::Info, "gpu", Stringizer() + "gpu memory heap type: " + vk::to_string(mem.memoryHeaps[i].flags).c_str() + ", capacity: " + (mem.memoryHeaps[i].size / 1024 / 1024) + " MB");
				}
			}

			{
				std::array<vk::DescriptorPoolSize, 6> sizes = {};
				sizes[0].type = vk::DescriptorType::eUniformBuffer;
				sizes[0].descriptorCount = 2'000;
				sizes[1].type = vk::DescriptorType::eUniformBufferDynamic;
				sizes[1].descriptorCount = 100;
				sizes[2].type = vk::DescriptorType::eStorageBuffer;
				sizes[2].descriptorCount = 2'000;
				sizes[3].type = vk::DescriptorType::eStorageBufferDynamic;
				sizes[3].descriptorCount = 100;
				sizes[4].type = vk::DescriptorType::eSampler;
				sizes[4].descriptorCount = 4'000;
				sizes[5].type = vk::DescriptorType::eSampledImage;
				sizes[5].descriptorCount = 4'000;
				vk::DescriptorPoolCreateInfo ci;
				ci.poolSizeCount = sizes.size();
				ci.pPoolSizes = sizes.data();
				ci.maxSets = 2'000;
				descriptorPool = device.createDescriptorPoolUnique(ci);
			}

			CAGE_LOG(SeverityEnum::Info, "gpu", "gpu device created");
		}

		Device::~Device()
		{
			CAGE_LOG(SeverityEnum::Info, "gpu", "destroying gpu device");

			{
				for (const auto &it : surfacesCollection)
					it->clear();
				surfacesCollection.clear();
			}

			{
				vmaDestroyAllocator(allocator);
				allocator = nullptr;
			}

			CAGE_LOG(SeverityEnum::Info, "gpu", "gpu device destroyed");
		}

		void Device::bootstrapInit(const gpu::GpuDeviceDescriptor &desc)
		{
			uint32 extsCnt = 0;
			const auto extsArr = glfwGetRequiredInstanceExtensions(&extsCnt);
			bootstrap.inst = handleResult(vkb::InstanceBuilder().require_api_version(1, 3).set_debug_callback(debugCallback).enable_extensions(extsCnt, extsArr).request_validation_layers().set_engine_name("cage").set_app_name(desc.label.str.data()).build());

			auto surf = getWindowGpuContext(desc.window);
			vk::PhysicalDeviceFeatures features10;
			features10.samplerAnisotropy = true;
			vk::PhysicalDeviceVulkan12Features features12;
			features12.descriptorIndexing = true;
			features12.shaderSampledImageArrayNonUniformIndexing = true;
			features12.descriptorBindingVariableDescriptorCount = true;
			features12.runtimeDescriptorArray = true;
			features12.bufferDeviceAddress = true;
			vk::PhysicalDeviceVulkan13Features features13;
			features13.synchronization2 = true;
			features13.dynamicRendering = true;
			bootstrap.phys = handleResult(vkb::PhysicalDeviceSelector(bootstrap.inst).set_minimum_version(1, 3).set_surface((VkSurfaceKHR)surf->data->surface).set_required_features(features10).set_required_features_12(features12).set_required_features_13(features13).select());

			bootstrap.dev = handleResult(vkb::DeviceBuilder(bootstrap.phys).build());

			bootstrap.qt = handleResult(bootstrap.dev.get_queue(vkb::QueueType::transfer));
			bootstrap.qg = handleResult(bootstrap.dev.get_queue(vkb::QueueType::graphics));
			bootstrap.qp = handleResult(bootstrap.dev.get_queue(vkb::QueueType::present));
		}

		Holder<privat::WindowGpuContext> Device::getWindowGpuContext(Window *window)
		{
			Holder<privat::WindowGpuContext> &context = privat::getWindowGpuContext(window);
			if (!context)
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "creating window gpu surface");
				auto s = std::make_shared<WindowGpuContextData>(bootstrap.inst.instance);
				VkSurfaceKHR rawSurface;
				const auto res = glfwCreateWindowSurface(bootstrap.inst.instance, privat::getGlfwWindow(window), nullptr, &rawSurface);
				if (res != VkResult::VK_SUCCESS)
				{
					CAGE_LOG_THROW(Stringizer() + "error code: " + res);
					CAGE_THROW_ERROR(Exception, "failed to create window gpu surface");
				}
				s->surface = vk::SurfaceKHR(rawSurface);
				surfacesCollection.push_back(s);
				context = systemMemory().createHolder<privat::WindowGpuContext>();
				context->data = s;
			}
			return context.share();
		}

		gpu::Texture Device::getWindowSurfaceTexture(Window *window)
		{
			const Vec2i res = window->resolution();
			if (res[0] <= 0 || res[1] <= 0)
				return {};
			auto ctx = getWindowGpuContext(window);
			if (ctx->data->resolution != res)
			{
				ctx->data->swapchain = handleResult(vkb::SwapchainBuilder(bootstrap.dev, (VkSurfaceKHR)ctx->data->surface).set_old_swapchain(ctx->data->swapchain).set_desired_min_image_count(2).set_desired_extent(res[0], res[1]).build());
				ctx->data->resolution = res;
				ctx->data->init();
			}
			// todo acquire texture
			return {};
		}
	}

	namespace gpu
	{
		Device newGpuDevice(const gpu::GpuDeviceDescriptor &desc)
		{
			return Device(systemMemory().createHolder<gpuImpl::Device>(desc));
		}
	}
}
