#include <memory>
#include <vector>

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>

#include <cage-engine/gpuInterface.h>

namespace cage
{
	namespace privat
	{
		struct GpuSurface;
	}

	namespace gpuImpl
	{
		struct GpuSurfaceData;

		class BindGroup : private Immovable
		{
		public:
			BindGroup(const gpu::BindGroupDescriptor &desc) {}
		};

		class BindGroupLayout : private Immovable
		{
		public:
			BindGroupLayout(const gpu::BindGroupLayoutDescriptor &desc) {}
		};

		class Buffer : private Immovable
		{
		public:
			vk::UniqueBuffer buffer;

			Buffer(const gpu::BufferDescriptor &desc) {}
		};

		class CommandBuffer : private Immovable
		{
		public:
			vk::UniqueCommandBuffer buffer;

			CommandBuffer(const CommandEncoder &commandEncoder) {}
		};

		class CommandEncoder : private Immovable
		{
		public:
			CommandEncoder(const gpu::CommandEncoderDescriptor &desc) {}
		};

		class Device : private Immovable
		{
			struct Bootstrap
			{
				vkb::Instance inst;
				vkb::PhysicalDevice phys;
				vkb::Device dev;
				VkQueue qt = nullptr;
				VkQueue qg = nullptr;
				VkQueue qp = nullptr;

				~Bootstrap();
			};
			Bootstrap bootstrap;

			std::vector<std::shared_ptr<GpuSurfaceData>> surfacesCollection;

		public:
			vk::Instance instance;
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			vk::Queue queueTransfer;
			vk::Queue queueGraphics;
			vk::Queue queuePresent;

			Device(const gpu::GpuDeviceDescriptor &desc);
			~Device();

			void bootstrapInit(const gpu::GpuDeviceDescriptor &desc);
			Holder<privat::GpuSurface> getWindowGpuSurface(Window *window);
		};

		class RenderPassEncoder : private Immovable
		{
		public:
			RenderPassEncoder(const CommandEncoder &commandEncoder, const gpu::RenderPassDescriptor &desc) {}
		};

		class RenderPipeline : private Immovable
		{
		public:
			RenderPipeline(const gpu::RenderPipelineDescriptor &desc) {}
		};

		class Sampler : private Immovable
		{
		public:
			vk::UniqueSampler sampler;

			Sampler(const gpu::SamplerDescriptor &desc) {}
		};

		class ShaderModule : private Immovable
		{
		public:
			vk::UniqueShaderModule shader;

			ShaderModule(const gpu::ShaderModuleDescriptor &desc) {}
		};

		class PipelineLayout : private Immovable
		{
		public:
			PipelineLayout(const gpu::PipelineLayoutDescriptor &desc) {}
		};

		class Texture : private Immovable
		{
		public:
			Texture(const gpu::TextureDescriptor &desc) {}
		};

		class TextureView : private Immovable
		{
		public:
			TextureView(const Texture &texture, const gpu::TextureViewDescriptor &desc) {}
		};
	}
}
