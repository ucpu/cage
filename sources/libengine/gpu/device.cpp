#include <cstdlib>
#include <cstring>
#include <stdlib.h>

#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS 1

#define GLFW_INCLUDE_VULKAN 1
#include <GLFW/glfw3.h>

#include "../window/private.h"
#include "gpu.h"

#include <cage-core/debug.h>
#include <cage-core/files.h>
#include <cage-core/profiling.h>
#include <cage-engine/window.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace cage
{
	namespace
	{
		void debugCallbackMaybeBreak(const VkDebugUtilsMessengerCallbackDataEXT *d)
		{
			if (d->messageIdNumber == 416909302)
			{
				// vkCreateImage(): pCreateInfo->pNext<VkExternalMemoryImageCreateInfo>.handleTypes is VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT (non-zero) but the initialLayout is VK_IMAGE_LAYOUT_PREINITIALIZED.
				// The Vulkan spec states: If the pNext chain includes a VkExternalMemoryImageCreateInfo or VkExternalMemoryImageCreateInfoNV structure whose handleTypes member is not 0, initialLayout must be VK_IMAGE_LAYOUT_UNDEFINED (https://docs.vulkan.org/spec/latest/chapters/resources.html#VUID-VkImageCreateInfo-pNext-01443)
				return;
			}

			if (d->messageIdNumber == 1180184443 && d->objectCount == 2 && std::strcmp(d->pObjects[1].pObjectName, "BlankEyeBuffer") == 0)
			{
				// vkQueueSubmit(): pSubmits[0] command buffer VkCommandBuffer 0x1c982a9c0d0 expects VkImage 0x2bb00000002bb[BlankEyeBuffer] (subresource: aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, mipLevel = 0, arrayLayer = 0) to be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL--instead, current layout is VK_IMAGE_LAYOUT_UNDEFINED.
				// The Vulkan spec states: If a descriptor with type equal to any of VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM, VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT is accessed as a result of this command, all image subresources identified by that descriptor must be in the image layout identified when the descriptor was written (https://docs.vulkan.org/spec/latest/chapters/drawing.html#VUID-vkCmdDraw-None-09600)
				return;
			}

			detail::debugBreakpoint();
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
		{
			SeverityEnum sev = SeverityEnum::Info;
			if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
				sev = SeverityEnum::Hint;
			if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				sev = SeverityEnum::Warning;
			if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				sev = SeverityEnum::Error;

			CAGE_LOG(sev, "vulkan", pCallbackData->pMessage);

			if (sev >= SeverityEnum::Error)
				debugCallbackMaybeBreak(pCallbackData);

			return VK_FALSE;
		}

		int environmentSetupImpl()
		{
			if (std::getenv("VK_DRIVER_FILES"))
			{
				CAGE_LOG(SeverityEnum::Info, "gpu", "detected env var: VK_DRIVER_FILES");
				return 0;
			}
			if (std::getenv("VK_ADD_DRIVER_FILES"))
			{
				CAGE_LOG(SeverityEnum::Info, "gpu", "detected env var: VK_ADD_DRIVER_FILES");
				return 0;
			}
			if (std::getenv("VK_ICD_FILENAMES"))
			{
				CAGE_LOG(SeverityEnum::Info, "gpu", "detected env var: VK_ICD_FILENAMES");
				return 0;
			}

#ifdef CAGE_SYSTEM_MAC
			{ // kosmic
				const String val = pathJoin(detail::pathExecutableDir(), "kosmic_icd.json");
				if (pathIsFile(val))
				{
					const auto r = ::setenv("VK_ADD_DRIVER_FILES", val.c_str(), 1);
					if (r != 0)
						CAGE_LOG(SeverityEnum::Warning, "gpu", "failed to update environment variable VK_ADD_DRIVER_FILES");
				}
			}
			{ // moltenvk
				const String val = pathJoin(detail::pathExecutableDir(), "moltenvk_icd.json");
				if (pathIsFile(val))
				{
					const auto r = ::setenv("VK_ADD_DRIVER_FILES", val.c_str(), 1);
					if (r != 0)
						CAGE_LOG(SeverityEnum::Warning, "gpu", "failed to update environment variable VK_ADD_DRIVER_FILES");
				}
			}
#endif // CAGE_SYSTEM_MAC

			return 0;
		}
	}

	namespace gpu
	{
		void logError(const vkb::Error &err)
		{
			CAGE_LOG(SeverityEnum::Note, "vulkan boostrap", err.type.message());
			for (const auto &it : err.detailed_failure_reasons)
				CAGE_LOG(SeverityEnum::Note, "vulkan boostrap", it);
			CAGE_LOG_THROW(Stringizer() + "error code: " + err.type.value());
		}

		template<>
		void ResourceInternal<vk::SwapchainKHR, Nothing>::destroy()
		{
			device->device.destroySwapchainKHR(value);
		}

		template<>
		void ResourceInternal<vk::Semaphore, Nothing>::destroy()
		{
			device->device.destroySemaphore(value);
		}

		void WindowGpuContextImpl::SwpImage::init()
		{
			if (initialized)
				return;
			ScopeLock lock(texture->image.device()->mutex);
			CommandEncoderImpl &enc = texture->image.device()->addCommands();
			enc.imageTransitionPermanent(texture, ImageStateEnum::Undefined, ImageStateEnum::Present);
			initialized = true;
		}

		WindowGpuContextImpl::WindowGpuContextImpl(vk::Instance instance_) : instance(instance_) {}

		WindowGpuContextImpl::~WindowGpuContextImpl() {}

		void WindowGpuContextImpl::init(DeviceImpl &device)
		{
			std::vector<VkImage> images = handleResult(swapchain.get_images());

			swpImages.clear();
			for (uint32 i = 0; i < images.size(); i++)
			{
				auto &f = swpImages.emplace_back(device);
				f.image = vk::Image(images[i]);
				f.texture = Texture(systemMemory().createHolder<TextureImpl>(device, f.image));
				f.texture->resolution = Vec3i(swapchain.extent.width, swapchain.extent.height, 1);
				f.texture->arrayLayersCount = f.texture->mipLevelsCount = 1;
				f.texture->dimension = TextureDimensionEnum::e2D;
				f.texture->format = convertTextureFormatInverse(vk::Format(swapchain.image_format));
				f.texture->usage = TextureUsageFlags::RenderAttachment; //swapchain.image_usage_flags;
				f.texture->image.setLabel((Stringizer() + "swapchainImage[" + i + "]").value);
				vk::SemaphoreCreateInfo sci;
				f.renderComplete = device.device.createSemaphore(sci);
				f.renderComplete.setLabel((Stringizer() + "renderComplete[" + i + "]").value);
			}
			imageIndex = 0;

			framesInFlight.clear();
			for (uint32 i = 0; i < 2; i++)
			{
				auto &f = framesInFlight.emplace_back(device);
				vk::SemaphoreCreateInfo sci;
				f.imageAcquired = device.device.createSemaphore(sci);
				f.imageAcquired.setLabel((Stringizer() + "imageAcquired[" + i + "]").value);
			}
			frameIndex = 0;
		}

		void WindowGpuContextImpl::clear()
		{
			swpImages.clear();
			framesInFlight.clear();
			vkb::destroy_swapchain(swapchain);
			instance.destroySurfaceKHR(surface);
			surface = nullptr;
		}

		void DeviceImpl::Bootstrap::environmentSetup()
		{
			static int dummy = environmentSetupImpl();
			(void)dummy;
		}

		void DeviceImpl::Bootstrap::setRequiredFeatures(vkb::InstanceBuilder &ib)
		{
			ib //
				.require_api_version(1, 3)
				.set_debug_callback(debugCallback)
#ifndef CAGE_DEPLOY
				.request_validation_layers()
#endif // !CAGE_DEPLOY
				.set_engine_name("cage");
		}

		void DeviceImpl::Bootstrap::setRequiredFeatures(vkb::PhysicalDeviceSelector &sel)
		{
			vk::PhysicalDeviceFeatures features10;
			features10.samplerAnisotropy = true;
			vk::PhysicalDeviceVulkan12Features features12;
			//features12.descriptorIndexing = true;
			//features12.shaderSampledImageArrayNonUniformIndexing = true;
			//features12.descriptorBindingVariableDescriptorCount = true;
			//features12.runtimeDescriptorArray = true;
			//features12.bufferDeviceAddress = true;
			vk::PhysicalDeviceVulkan13Features features13;
			features13.synchronization2 = true;
			features13.dynamicRendering = true;
			sel //
				.set_minimum_version(1, 3)
				.set_required_features(features10)
				.set_required_features_12(features12)
				.set_required_features_13(features13);
		}

		DeviceImpl::Bootstrap::Bootstrap()
		{
			environmentSetup();
		}

		DeviceImpl::Bootstrap::Bootstrap(Bootstrap &&other)
		{
			std::swap(inst, other.inst);
			std::swap(phys, other.phys);
			std::swap(dev, other.dev);
			std::swap(q, other.q);
		}

		DeviceImpl::Bootstrap::~Bootstrap()
		{
			vkb::destroy_device(dev);
			vkb::destroy_instance(inst);
		}

		DeviceImpl::DeviceImpl(Bootstrap &&bootstrap) : bootstrap(std::move(bootstrap))
		{
			commonInitialization();
		}

		DeviceImpl::DeviceImpl(const GpuDeviceDescriptor &desc)
		{
			CAGE_LOG(SeverityEnum::Info, "gpu", "creating gpu device");

			{
				uint32 extsCnt = 0;
				const auto extsArr = glfwGetRequiredInstanceExtensions(&extsCnt);
				vkb::InstanceBuilder ib;
				bootstrap.setRequiredFeatures(ib);
				ib.enable_extensions(extsCnt, extsArr);
				ib.set_app_name(desc.label.data());
				bootstrap.inst = handleResult(ib.build());
			}

			{
				vkb::PhysicalDeviceSelector sel(bootstrap.inst);
				bootstrap.setRequiredFeatures(sel);
				sel.set_surface((VkSurfaceKHR)getWindowGpuContext(desc.window)->data->surface);
				bootstrap.phys = handleResult(sel.select());
			}

			bootstrap.dev = handleResult(vkb::DeviceBuilder(bootstrap.phys).build());
			bootstrap.q = handleResult(bootstrap.dev.get_queue(vkb::QueueType::graphics));
			commonInitialization();

			CAGE_LOG(SeverityEnum::Info, "gpu", "gpu device created");
		}

		DeviceImpl::~DeviceImpl()
		{
			CAGE_LOG(SeverityEnum::Info, "gpu", "destroying gpu device");

			try
			{
				device.waitIdle();
			}
			catch (...)
			{
				// nothing
			}

			try
			{
				for (const auto &it : surfacesCollection)
					it->clear();
				surfacesCollection.clear();
			}
			catch (...)
			{
				// nothing
			}

			try
			{
				additionalCommands.clear();
				for (uint32 i = 0; i < 10; i++)
					applyDeferredDestructions();
			}
			catch (...)
			{
				// nothing
			}

			try
			{
				commandPools.clear();
				for (uint32 i = 0; i < 10; i++)
					applyDeferredDestructions();
			}
			catch (...)
			{
				// nothing
			}

			try
			{
				vmaDestroyAllocator(allocator);
				allocator = nullptr;
			}
			catch (...)
			{
				// nothing
			}

			CAGE_LOG(SeverityEnum::Info, "gpu", "gpu device destroyed");
		}

		void DeviceImpl::commonInitialization()
		{
			{
				instance = bootstrap.inst.instance;
				physicalDevice = bootstrap.phys.physical_device;
				device = bootstrap.dev.device;
				queue = bootstrap.q;
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
				//info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
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
				sizes[0].descriptorCount = 10'000;
				sizes[1].type = vk::DescriptorType::eUniformBufferDynamic;
				sizes[1].descriptorCount = 4'000;
				sizes[2].type = vk::DescriptorType::eStorageBuffer;
				sizes[2].descriptorCount = 10'000;
				sizes[3].type = vk::DescriptorType::eStorageBufferDynamic;
				sizes[3].descriptorCount = 4'000;
				sizes[4].type = vk::DescriptorType::eSampler;
				sizes[4].descriptorCount = 10'000;
				sizes[5].type = vk::DescriptorType::eSampledImage;
				sizes[5].descriptorCount = 10'000;
				vk::DescriptorPoolCreateInfo ci;
				ci.flags |= vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
				ci.poolSizeCount = sizes.size();
				ci.pPoolSizes = sizes.data();
				ci.maxSets = 10'000;
				descriptorPool = device.createDescriptorPoolUnique(ci);
			}

			{
				vk::FenceCreateInfo info;
				info.flags = vk::FenceCreateFlagBits::eSignaled;
				framesFences[0] = device.createFenceUnique(info);
				framesFences[1] = device.createFenceUnique(info);
			}

			{
				const auto queueFamilyIndex = handleResult(bootstrap.dev.get_queue_index(vkb::QueueType::graphics));
				capabilities.timestampsAvailable = bootstrap.dev.queue_families[queueFamilyIndex].timestampValidBits > 0;
				if (capabilities.timestampsAvailable)
					capabilities.timestampsConvert = bootstrap.phys.properties.limits.timestampPeriod;
				capabilities.maxAnisotropy = bootstrap.phys.properties.limits.maxSamplerAnisotropy;
			}
		}

		void DeviceImpl::applyDeferredDestructions()
		{
			ProfilingScope profiling("deferred destruction");
			profiling.set(Stringizer() + "destructions: " + deferredDestructions.back().size());

			deferredDestructions.back().clear();
			std::swap(deferredDestructions[2], deferredDestructions[1]);
			std::swap(deferredDestructions[1], deferredDestructions[0]);

			while (!disposingTasks.empty() && disposingTasks[0]->done())
				disposingTasks.erase(disposingTasks.begin());
		}

		CommandEncoderImpl &DeviceImpl::addCommands()
		{
			if (!additionalCommands)
				additionalCommands = systemMemory().createHolder<CommandEncoderImpl>(*this, CommandEncoderDescriptor{ .label = "additionalCommands" });
			return *additionalCommands;
		}

		Holder<privat::WindowGpuContext> DeviceImpl::getWindowGpuContext(Window *window)
		{
			Holder<privat::WindowGpuContext> &context = privat::getWindowGpuContext(window);
			if (!context)
			{
				CAGE_LOG(SeverityEnum::Info, "gpu", "creating window gpu surface");
				auto s = std::make_shared<WindowGpuContextImpl>(bootstrap.inst.instance);
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

		void DeviceImpl::setVsyncPreference(bool vsync, bool tripleBuffer)
		{
			const vk::PresentModeKHR pm = vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;
			if (preferredPresentation == pm && preferredTripleBuffering == tripleBuffer)
				return; // no change needed
			preferredPresentation = pm;
			preferredTripleBuffering = tripleBuffer;
			for (auto &it : surfacesCollection)
				it->resolution = {}; // refresh the swapchain next frame
		}

		double DeviceImpl::getTimestampsConversion() const
		{
			return capabilities.timestampsConvert;
		}

		void DeviceImpl::submitAndPresent(PointerRange<const CommandBuffer> buffers_, PointerRange<WindowPresentationDescriptor> windows_)
		{
			struct WindowEntry
			{
				Window *window = nullptr;
				Texture *texture = nullptr;
				Holder<privat::WindowGpuContext> ctxHolder;
				WindowGpuContextImpl *ctx = nullptr;
				Vec2i resolution;

				WindowGpuContextImpl *operator->()
				{
					CAGE_ASSERT(ctx);
					return ctx;
				}
			};
			ankerl::svector<WindowEntry, 1> windows;
			windows.reserve(windows_.size());
			for (auto &w : windows_)
			{
				WindowEntry e;
				e.window = w.window;
				e.texture = &w.texture;
				e.ctxHolder = getWindowGpuContext(e.window);
				if (e.ctxHolder && e.ctxHolder->data)
				{
					e.ctx = e.ctxHolder->data.get();
					e.resolution = e.window->resolution();
				}
				windows.push_back(std::move(e));
			}

			// submit
			{
				ScopeLock lock(mutex); // must protect additionalCommands and queue submit
				const ProfilingScope profiling("submit");
				ankerl::svector<vk::CommandBufferSubmitInfo, 30> cmds;
				CommandBuffer acb; // must outlive the submission
				if (additionalCommands)
				{
					acb = additionalCommands->finishEncoding();
					cmds.push_back(vk::CommandBufferSubmitInfo(acb->buffer));
					additionalCommands.clear();
				}
				for (auto &it : buffers_)
					cmds.push_back(vk::CommandBufferSubmitInfo(it->buffer));

				ankerl::svector<vk::SemaphoreSubmitInfo, 1> ias, rcs;
				for (auto &w : windows)
				{
					if (!w.ctx || !w->acquired)
						continue;
					CAGE_ASSERT(w->img().initialized);
					ias.push_back(vk::SemaphoreSubmitInfo(w->frm().imageAcquired, 0, vk::PipelineStageFlagBits2::eColorAttachmentOutput));
					rcs.push_back(vk::SemaphoreSubmitInfo(w->img().renderComplete, 0, vk::PipelineStageFlagBits2::eAllGraphics));
				}

				vk::SubmitInfo2 submitInfo;
				submitInfo.waitSemaphoreInfoCount = ias.size();
				submitInfo.pWaitSemaphoreInfos = ias.data();
				submitInfo.commandBufferInfoCount = cmds.size();
				submitInfo.pCommandBufferInfos = cmds.data();
				submitInfo.signalSemaphoreInfoCount = rcs.size();
				submitInfo.pSignalSemaphoreInfos = rcs.data();
				check("resetFences", device.resetFences(1, &*framesFences[0]));
				check("submit", queue.submit2(1, &submitInfo, *framesFences[0]));

				// advance frame index
				std::swap(framesFences[0], framesFences[1]);
				for (auto &w : windows)
					if (w.ctx)
						w->frameIndex = (w->frameIndex + 1) % 2;
			}

			// present
			{
				const ProfilingScope profiling("present");
				ankerl::svector<vk::Semaphore, 1> rcs;
				ankerl::svector<vk::SwapchainKHR, 1> sws;
				ankerl::svector<uint32, 1> ids;
				for (auto &w : windows)
				{
					if (!w.ctx || !w->acquired)
						continue;
					CAGE_ASSERT(w->img().initialized);
					rcs.push_back(w->img().renderComplete);
					sws.push_back((vk::SwapchainKHR)w->swapchain.swapchain);
					ids.push_back(w->imageIndex);
				}

				if (!sws.empty())
				{
					vk::PresentInfoKHR info;
					info.waitSemaphoreCount = rcs.size();
					info.pWaitSemaphores = rcs.data();
					info.swapchainCount = sws.size();
					info.pSwapchains = sws.data();
					info.pImageIndices = ids.data();
					auto r = queue.presentKHR(info);
					switch (r)
					{
						case vk::Result::eSuccess:
							break;
						case vk::Result::eSuboptimalKHR:
						case vk::Result::eErrorOutOfDateKHR:
						{
							for (auto &it : windows)
								if (it.ctx)
									it.ctx->resolution = {}; // refresh next frame
							break;
						}
						default:
						{
							check("presentKHR", r);
							break;
						}
					}
				}
			}

			{ // destroy pending destructions
				ScopeLock lock(mutex);
				applyDeferredDestructions();
			}

			// wait fence
			{
				const ProfilingScope profiling("wait fence");
				check("waitForFences", device.waitForFences(1, &*framesFences[0], true, m));
			}

			// acquire next
			{
				const ProfilingScope profiling("acquire image");
				for (auto &w : windows)
				{
					if (!w.ctx)
						continue;
					w->acquired = false;
					if (w.resolution[0] <= 0 || w.resolution[1] <= 0)
						continue;

					if (w->resolution != w.resolution)
					{
						const ProfilingScope profiling("swapchain");
						CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "updating swapchain, resolution: " + w.resolution + ", presentation: " + vk::to_string(preferredPresentation).c_str() + ", triple buffering: " + preferredTripleBuffering);
						ResourceHandle<vk::SwapchainKHR> old(*this);
						old = vk::SwapchainKHR(w->swapchain.swapchain);
						w->swapchain = handleResult(vkb::SwapchainBuilder(bootstrap.dev, (VkSurfaceKHR)w->surface) //
														.set_old_swapchain(w->swapchain)
														.set_desired_min_image_count(preferredTripleBuffering ? 3 : 2)
														.set_desired_extent(w.resolution[0], w.resolution[1])
														.set_desired_present_mode((VkPresentModeKHR)preferredPresentation)
														.build());
						w->resolution = w.resolution;
						w->init(*this);
					}

					auto r = device.acquireNextImageKHR((vk::SwapchainKHR)w->swapchain.swapchain, m, w->frm().imageAcquired);
					switch (vk::Result(r.result))
					{
						case vk::Result::eSuccess:
						{
							w->imageIndex = r.value;
							*w.texture = w->img().texture;
							break;
						}
						case vk::Result::eSuboptimalKHR:
						{
							w->imageIndex = r.value;
							*w.texture = w->img().texture;
							w->resolution = {}; // refresh next frame
							break;
						}
						case vk::Result::eErrorOutOfDateKHR:
						{
							w->resolution = {}; // refresh next frame
							break;
						}
						default:
						{
							check("acquireNextImageKHR", r.result);
							break;
						}
					}
					w->img().init();
					w->acquired = true;
				}
			}
		}

		void DeviceImpl::submit(PointerRange<const CommandBuffer> buffers_)
		{
			const ProfilingScope profiling("submit");

			ankerl::svector<vk::CommandBufferSubmitInfo, 10> cmds;
			CommandBuffer acb; // must outlive the submission
			if (additionalCommands)
			{
				acb = additionalCommands->finishEncoding();
				cmds.push_back(vk::CommandBufferSubmitInfo(acb->buffer));
				additionalCommands.clear();
			}
			for (auto &it : buffers_)
				cmds.push_back(vk::CommandBufferSubmitInfo(it->buffer));

			vk::SubmitInfo2 submitInfo;
			submitInfo.commandBufferInfoCount = cmds.size();
			submitInfo.pCommandBufferInfos = cmds.data();
			check("submit", queue.submit2(1, &submitInfo, nullptr));
		}

		void DeviceImpl::waitDeviceIdle()
		{
			const ProfilingScope profiling("waitDeviceIdle");
			device.waitIdle();
		}

		Device newGpuDevice(const GpuDeviceDescriptor &desc)
		{
			return Device(systemMemory().createHolder<DeviceImpl>(desc));
		}
	}
}
