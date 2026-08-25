#include <GLFW/glfw3.h>

#include "../gpu/gpu.h"
#include <cage-engine/gpuInterface.h>

#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>

namespace cage
{
	namespace
	{
		template<class T>
		void init(T &t, XrStructureType type)
		{
			detail::memset(&t, 0, sizeof(t));
			t.type = type;
		}

		struct Builder
		{
			const XrInstance instance;
			const XrSystemId systemId;
			XrSession &session;
			gpu::Device &gpuDevice;
			const gpu::GpuDeviceDescriptor &gpuDeviceDescriptor;
			std::vector<char> ie, de;

			CAGE_FORCE_INLINE void check(XrResult result) const
			{
				if (XR_SUCCEEDED(result))
					return;

				if (instance)
				{
					char s[XR_MAX_RESULT_STRING_SIZE] = {};
					xrResultToString(instance, result, s);
					if (s[0])
						CAGE_LOG_THROW(s);
				}

				CAGE_THROW_ERROR(SystemError, "openxr error", result);
			}

			template<class FncType>
			std::vector<char> loadExtensionList(const char *fncName)
			{
				FncType fnc = nullptr;
				check(xrGetInstanceProcAddr(instance, fncName, (PFN_xrVoidFunction *)&fnc));
				uint32 sz = 0;
				check(fnc(instance, systemId, 0, &sz, nullptr));
				std::vector<char> buf;
				buf.reserve(sz + 1);
				buf.resize(sz);
				check(fnc(instance, systemId, sz, &sz, buf.data()));
				buf.push_back(' ');
				return buf;
			}

			void addInstanceExtensions(vkb::InstanceBuilder &ib)
			{
				ie = loadExtensionList<PFN_xrGetVulkanInstanceExtensionsKHR>("xrGetVulkanInstanceExtensionsKHR");
				auto &buf = ie;
				const char *p = buf.data();
				for (char &it : buf)
				{
					if (it == ' ')
					{
						it = '\0';
						ib.enable_extension(p);
						p = &it;
						p++;
					}
				}
			}

			void addDeviceExtensions(vkb::PhysicalDeviceSelector &sel)
			{
				de = loadExtensionList<PFN_xrGetVulkanDeviceExtensionsKHR>("xrGetVulkanDeviceExtensionsKHR");
				auto &buf = de;
				const char *p = buf.data();
				for (char &it : buf)
				{
					if (it == ' ')
					{
						it = '\0';
						sel.add_required_extension(p);
						p = &it;
						p++;
					}
				}
			}

			void additionalRequiredFeatures(vkb::PhysicalDeviceSelector &sel)
			{
				vk::PhysicalDeviceFeatures features10;
				features10.geometryShader = true;
				vk::PhysicalDeviceVulkan12Features features12;
				features12.shaderOutputViewportIndex = true;
				features12.shaderOutputLayer = true;
				features12.timelineSemaphore = true;
				features12.descriptorIndexing = true;
				features12.bufferDeviceAddress = true;
				vk::PhysicalDeviceVulkan13Features features13;
				features13.dynamicRendering = true;
				features13.synchronization2 = true;
				sel.set_required_features(features10);
				sel.set_required_features_12(features12);
				sel.set_required_features_13(features13);
			}

			void initDevice()
			{
				gpu::DeviceImpl::Bootstrap bootstrap;

				// vk instance
				{
					vkb::InstanceBuilder ib;
					gpu::DeviceImpl::Bootstrap::setRequiredFeatures(ib);
					addInstanceExtensions(ib);
					ib.set_app_name(gpuDeviceDescriptor.label.data());
					bootstrap.inst = gpu::handleResult(ib.build());
				}

				// xr vulkan required version
				{
					PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR = nullptr;
					check(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&xrGetVulkanGraphicsRequirementsKHR));
					XrGraphicsRequirementsVulkan2KHR reqs;
					init(reqs, XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR);
					check(xrGetVulkanGraphicsRequirementsKHR(instance, systemId, &reqs));
				}

				// vk physical device
				{
					PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR = nullptr;
					check(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction *)&xrGetVulkanGraphicsDeviceKHR));
					VkPhysicalDevice xrPhysicalDevice = VK_NULL_HANDLE;
					check(xrGetVulkanGraphicsDeviceKHR(instance, systemId, bootstrap.inst, &xrPhysicalDevice));
					bootstrap.phys = [&]()
					{
						vkb::PhysicalDeviceSelector sel(bootstrap.inst);
						gpu::DeviceImpl::Bootstrap::setRequiredFeatures(sel);
						addDeviceExtensions(sel);
						additionalRequiredFeatures(sel);
						sel.defer_surface_initialization();
						auto list = gpu::handleResult(sel.select_devices());
						for (const auto &it : list)
						{
							if (it.physical_device == xrPhysicalDevice)
								return it;
						}
						CAGE_THROW_ERROR(Exception, "vulkan bootstrap did not provide physical device matching the one requested by openxr");
					}();
				}

				bootstrap.dev = gpu::handleResult(vkb::DeviceBuilder(bootstrap.phys).build());
				bootstrap.q = gpu::handleResult(bootstrap.dev.get_queue(vkb::QueueType::graphics));
				gpuDevice = gpu::Device(systemMemory().createHolder<gpu::DeviceImpl>(std::move(bootstrap)));
			}

			void initSession()
			{
				XrGraphicsBindingVulkanKHR binding;
				init(binding, XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR);
				binding.instance = gpuDevice->bootstrap.inst;
				binding.physicalDevice = gpuDevice->bootstrap.phys;
				binding.device = gpuDevice->bootstrap.dev;
				binding.queueFamilyIndex = gpuDevice->bootstrap.dev.get_queue_index(vkb::QueueType::graphics).value();
				binding.queueIndex = 0;
				XrSessionCreateInfo info;
				init(info, XR_TYPE_SESSION_CREATE_INFO);
				info.next = &binding;
				info.systemId = systemId;
				check(xrCreateSession(instance, &info, &session));
			}
		};
	}

	namespace privat
	{
		void plaformInitSession(XrInstance instance, XrSystemId systemId, XrSession &session, gpu::Device &gpuDevice, const gpu::GpuDeviceDescriptor &gpuDeviceDescriptor)
		{
			CAGE_LOG(SeverityEnum::Info, "virtualReality", "initializing openxr platform and session");
			Builder builder{ instance, systemId, session, gpuDevice, gpuDeviceDescriptor };
			builder.initDevice();
			builder.initSession();
			CAGE_LOG(SeverityEnum::Info, "virtualReality", "openxr platform and session initialized");
		}
	}
}
