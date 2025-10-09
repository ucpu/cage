#include <array>
#include <cstring> // std::strlen

#include <dawn/native/DawnNative.h>
#include <webgpu/webgpu_cpp.h>

#include <cage-core/debug.h>
#include <cage-engine/graphicsDevice.h>

namespace cage
{
	namespace
	{
		String conv(wgpu::StringView v)
		{
			// todo check for truncation
			if (!v.data)
				return {};
			if (v.length == wgpu::kStrlen)
				return String({ v.data, v.data + std::strlen(v.data) });
			return String({ v.data, v.data + v.length });
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

			GraphicsDeviceImpl()
			{
				{
					dawn::native::DawnInstanceDescriptor ex;
					ex.SetLoggingCallback(logFromInstance);
#ifdef CAGE_DEBUG
					ex.backendValidationLevel = dawn::native::BackendValidationLevel::Partial;
#endif // CAGE_DEBUG
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
					opts.compatibleSurface = nullptr; // todo

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
		};
	}

	Holder<GraphicsDevice> newGraphicsDevice()
	{
		return systemMemory().createImpl<GraphicsDevice, GraphicsDeviceImpl>();
	}
}
