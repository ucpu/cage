#include <cstring>

#define GLFW_INCLUDE_VULKAN 1
#include <GLFW/glfw3.h>

#include "../window/private.h"
#include "gpu.h"

#include <cage-core/debug.h>

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

	namespace privat
	{
		struct GpuSurface : private Immovable
		{
			// the window owns its surface, but the device can destroy all the surfaces in its destructor
			std::shared_ptr<gpuImpl::GpuSurfaceData> data;
		};
	}

	namespace gpuImpl
	{
		struct GpuSurfaceData : private Immovable
		{
			vk::SurfaceKHR surface;
			Vec2i resolution;
			bool presentable = false;
		};

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

			CAGE_LOG(SeverityEnum::Info, "gpu", "gpu device created");
		}

		Device::~Device()
		{
			CAGE_LOG(SeverityEnum::Info, "gpu", "destroying gpu device");

			{
				for (const auto &it : surfacesCollection)
				{
					if (it->surface)
						instance.destroySurfaceKHR(it->surface);
					it->surface = nullptr;
				}
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

			auto surf = getWindowGpuSurface(desc.window);
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

		Holder<privat::GpuSurface> Device::getWindowGpuSurface(Window *window)
		{
			Holder<privat::GpuSurface> &context = privat::getWindowGpuSurface(window);
			if (!context)
			{
				CAGE_LOG(SeverityEnum::Info, "graphics", "creating window gpu surface");
				auto s = std::make_shared<GpuSurfaceData>();
				VkSurfaceKHR rawSurface;
				const auto res = glfwCreateWindowSurface(bootstrap.inst.instance, privat::getGlfwWindow(window), nullptr, &rawSurface);
				if (res != VkResult::VK_SUCCESS)
				{
					CAGE_LOG_THROW(Stringizer() + "error code: " + res);
					CAGE_THROW_ERROR(Exception, "failed to create window gpu surface");
				}
				s->surface = vk::SurfaceKHR(rawSurface);
				surfacesCollection.push_back(s);
				context = systemMemory().createHolder<privat::GpuSurface>();
				context->data = s;
			}
			return context.share();
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
