#include <cstring>

#define GLFW_INCLUDE_VULKAN 1
#include <GLFW/glfw3.h>
//#define VK_NO_PROTOTYPES

#include "../window/private.h"
#include "gpu.h"

#include <cage-core/debug.h>

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
				//detail::debugBreakpoint();
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

		//template<class T, class Src>
		//CAGE_FORCE_INLINE vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> makeUnique(Src &src)
		//{
		//	return vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>(T(src), vk::UniqueHandleTraits<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::deleter());
		//}

		template<class T, class S>
		T makeHppHandle(S &s)
		{
			return T(s);
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

		Device::Device(Window *window)
		{
			CAGE_LOG(SeverityEnum::Info, "gpu", "creating gpu device");
			bootstrapInit(window);
			instance = makeHppHandle<vk::Instance>(bootstrap.inst.instance);
			physicalDevice = makeHppHandle<vk::PhysicalDevice>(bootstrap.phys.physical_device);
			device = makeHppHandle<vk::Device>(bootstrap.dev.device);
			queueTransfer = makeHppHandle<vk::Queue>(bootstrap.qt);
			queueGraphics = makeHppHandle<vk::Queue>(bootstrap.qg);
			queuePresent = makeHppHandle<vk::Queue>(bootstrap.qp);
			CAGE_LOG(SeverityEnum::Info, "gpu", "gpu device created");
		}

		Device::~Device()
		{
			for (const auto &it : surfacesCollection)
			{
				// todo destroy the surface
				//if (it->surface)
				//	vkDestroySurfaceKHR(instance, it->surface, nullptr);
				it->surface = nullptr;
			}
		}

		void Device::bootstrapInit(Window *window)
		{
			uint32 extsCnt = 0;
			const auto extsArr = glfwGetRequiredInstanceExtensions(&extsCnt);
			bootstrap.inst = handleResult(vkb::InstanceBuilder().require_api_version(1, 3).set_debug_callback(debugCallback).enable_extensions(extsCnt, extsArr).enable_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME).request_validation_layers().build());
			auto surf = getWindowGpuSurface(window);
			bootstrap.phys = handleResult(vkb::PhysicalDeviceSelector(bootstrap.inst).set_surface((VkSurfaceKHR)surf->data->surface).select());
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
		Device newGpuDevice(Window *window)
		{
			return Device(systemMemory().createHolder<gpuImpl::Device>(window));
		}
	}
}
