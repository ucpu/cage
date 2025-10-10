#include <array>
#include <cstring> // std::strlen

#include <dawn/native/DawnNative.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "../window/private.h"

#include <cage-core/debug.h>
#include <cage-engine/graphicsDevice.h>
#include <cage-engine/window.h>

namespace cage
{
	Holder<Texture> adaptWgpuTexture(wgpu::Texture texture);

	struct GraphicsContext
	{
		wgpu::Surface surface;
		Vec2i resolution;
		bool presentable = false;
	};

	namespace
	{
		String conv(wgpu::StringView v)
		{
			if (!v.data)
				return {};
			uint32 len = v.length;
			if (v.length == wgpu::kStrlen)
				len = std::strlen(v.data);
			if (len > 500)
				len = 500;
			return String({ v.data, v.data + len });
		}

		void logFromInstance(wgpu::LoggingType type, wgpu::StringView message)
		{
			SeverityEnum severity = SeverityEnum::Critical;
			switch (type)
			{
				case wgpu::LoggingType::Verbose:
					severity = SeverityEnum::Note;
					break;
				case wgpu::LoggingType::Info:
					severity = SeverityEnum::Info;
					break;
				case wgpu::LoggingType::Warning:
					severity = SeverityEnum::Warning;
					break;
				case wgpu::LoggingType::Error:
					severity = SeverityEnum::Error;
					break;
			}
			CAGE_LOG(severity, "wgpu", conv(message));
		}

		void lostFromDevice(const wgpu::Device &device, wgpu::DeviceLostReason reason, wgpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Info, "graphics", "device lost");
			CAGE_LOG(SeverityEnum::Note, "wgpu", conv(message));
		}

		void errorFromDevice(const wgpu::Device &device, wgpu::ErrorType error, wgpu::StringView message)
		{
			CAGE_LOG(SeverityEnum::Error, "graphics", "device error");
			CAGE_LOG(SeverityEnum::Error, "wgpu", conv(message));
			detail::debugBreakpoint();
		}

		class GraphicsDeviceImpl : public GraphicsDevice
		{
		public:
			wgpu::Instance instance;
			wgpu::Device device;
			wgpu::Queue queue;

			GraphicsDeviceImpl(const GraphicsDeviceCreateConfig &config)
			{
				{
					dawn::native::DawnInstanceDescriptor ex;
					ex.SetLoggingCallback(logFromInstance);
					wgpu::InstanceDescriptor desc;
					desc.nextInChain = &ex;
					std::array<wgpu::InstanceFeatureName, 2> features = { wgpu::InstanceFeatureName::TimedWaitAny, wgpu::InstanceFeatureName::ShaderSourceSPIRV };
					desc.requiredFeatureCount = features.size();
					desc.requiredFeatures = features.data();

					instance = wgpu::CreateInstance(&desc);
					if (!instance)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu instance");
				}

				wgpu::Adapter adapter;
				{
					wgpu::RequestAdapterOptions opts;
					opts.backendType = wgpu::BackendType::Vulkan;
					opts.powerPreference = wgpu::PowerPreference::HighPerformance;
					if (config.compatibility)
						opts.compatibleSurface = getContext(config.compatibility)->surface;

					wgpu::Future future = instance.RequestAdapter(&opts, wgpu::CallbackMode::WaitAnyOnly,
						[&](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter_, wgpu::StringView message)
						{
							adapter = adapter_;
							CAGE_LOG(SeverityEnum::Info, "wgpu", conv(message));
						});
					instance.WaitAny(future, m);
					if (!adapter)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu adapter");

					wgpu::AdapterInfo info;
					adapter.GetInfo(&info);
					CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter vendor: " + conv(info.vendor));
					CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter device: " + conv(info.device));
					CAGE_LOG(SeverityEnum::Info, "graphics", Stringizer() + "adapter description: " + conv(info.description));
				}

				{
					wgpu::DeviceDescriptor desc;
					desc.SetDeviceLostCallback(wgpu::CallbackMode::AllowProcessEvents, lostFromDevice);
					desc.SetUncapturedErrorCallback(errorFromDevice);

					device = adapter.CreateDevice(&desc);
					if (!device)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu device");
				}

				{
					queue = device.GetQueue();
					if (!queue)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu queue");
				}
			}

			GraphicsContext *getContext(Window *window)
			{
				Holder<GraphicsContext> &context = getGlfwContext(window);
				if (!context)
					context = systemMemory().createHolder<GraphicsContext>();
				if (!context->surface)
				{
					context->surface = wgpu::glfw::CreateSurfaceForWindow(instance, getGlfwWindow(window));
					if (!context->surface)
						CAGE_THROW_ERROR(Exception, "failed to create wgpu surface from window");
				}
				return +context;
			}

			void processEvents() { device.Tick(); }

			Holder<Texture> nextFrame(Window *window)
			{
				GraphicsContext *context = getContext(window);

				const Vec2i res = window->resolution();
				CAGE_ASSERT(res[0] >= 0 && res[1] >= 0);
				if (res[0] * res[1] == 0)
					return {};

				if (res != context->resolution)
				{
					context->presentable = false;
					wgpu::SurfaceConfiguration cfg;
					cfg.device = device;
					cfg.width = res[0];
					cfg.height = res[1];
					cfg.presentMode = wgpu::PresentMode::Mailbox;
					cfg.format = wgpu::TextureFormat::RGBA8Unorm;
					//cfg.usage = wgpu::TextureUsage::RenderAttachment;
					context->surface.Configure(&cfg);
					context->resolution = res;
				}

				if (context->presentable)
					context->surface.Present();

				wgpu::SurfaceTexture tex;
				context->surface.GetCurrentTexture(&tex);
				switch (tex.status)
				{
					case wgpu::SurfaceGetCurrentTextureStatus::Error:
					case wgpu::SurfaceGetCurrentTextureStatus::Lost:
						context->presentable = false;
						CAGE_THROW_ERROR(Exception, "surface lost/error");
					default:
						// todo handle other options
						break;
				}

				CAGE_ASSERT(tex.texture);
				context->presentable = true;
				return adaptWgpuTexture(tex.texture);
			}
		};
	}

	Holder<GraphicsDevice> newGraphicsDevice(const GraphicsDeviceCreateConfig &config)
	{
		return systemMemory().createImpl<GraphicsDevice, GraphicsDeviceImpl>(config);
	}

	void GraphicsDevice::processEvents()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		impl->processEvents();
	}

	Holder<Texture> GraphicsDevice::nextFrame(Window *window)
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return impl->nextFrame(window);
	}

	wgpu::Device GraphicsDevice::nativeDevice()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return impl->device;
	}

	wgpu::Queue GraphicsDevice::nativeQueue()
	{
		GraphicsDeviceImpl *impl = (GraphicsDeviceImpl *)this;
		return impl->queue;
	}
}
