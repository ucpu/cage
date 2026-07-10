#include <memory>
#include <vector>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
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
			BindGroup(const Device &device, const gpu::BindGroupDescriptor &desc);
			~BindGroup();
		};

		class BindGroupLayout : private Immovable
		{
		public:
			BindGroupLayout(const Device &device, const gpu::BindGroupLayoutDescriptor &desc);
			~BindGroupLayout();
		};

		class Buffer : private Immovable
		{
		public:
			vk::UniqueBuffer buffer;
			uint64 size = 0;
			gpu::BufferUsage usage = gpu::BufferUsage::Undefined;

			Buffer(const Device &device, const gpu::BufferDescriptor &desc);
			~Buffer();
		};

		class CommandBuffer : private Immovable
		{
		public:
			vk::UniqueCommandBuffer buffer;

			CommandBuffer(const CommandEncoder &commandEncoder);
			~CommandBuffer();
		};

		class CommandEncoder : private Immovable
		{
		public:
			CommandEncoder(const Device &device, const gpu::CommandEncoderDescriptor &desc);
			~CommandEncoder();
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
			VmaAllocator allocator = nullptr;

			Device(const gpu::GpuDeviceDescriptor &desc);
			~Device();

			void bootstrapInit(const gpu::GpuDeviceDescriptor &desc);
			Holder<privat::GpuSurface> getWindowGpuSurface(Window *window);
		};

		class RenderPassEncoder : private Immovable
		{
		public:
			RenderPassEncoder(const CommandEncoder &commandEncoder, const gpu::RenderPassDescriptor &desc);
			~RenderPassEncoder();
		};

		class RenderPipeline : private Immovable
		{
		public:
			RenderPipeline(const Device &device, const gpu::RenderPipelineDescriptor &desc);
			~RenderPipeline();
		};

		class Sampler : private Immovable
		{
		public:
			vk::UniqueSampler sampler;

			Sampler(const Device &device, const gpu::SamplerDescriptor &desc);
			~Sampler();
		};

		class ShaderModule : private Immovable
		{
		public:
			vk::UniqueShaderModule shader;

			ShaderModule(const Device &device, const gpu::ShaderModuleDescriptor &desc);
			~ShaderModule();
		};

		class PipelineLayout : private Immovable
		{
		public:
			PipelineLayout(const Device &device, const gpu::PipelineLayoutDescriptor &desc);
			~PipelineLayout();
		};

		class Texture : private Immovable
		{
		public:
			Vec3i resolution;
			uint32 mipLevels = 0;
			gpu::TextureDimension dimension = gpu::TextureDimension::Undefined;
			gpu::TextureFormat format = gpu::TextureFormat::Undefined;

			Texture(const Device &device, const gpu::TextureDescriptor &desc);
			~Texture();
		};

		class TextureView : private Immovable
		{
		public:
			TextureView(const Texture &texture, const gpu::TextureViewDescriptor &desc);
			~TextureView();
		};

		CAGE_FORCE_INLINE void check(const char *what, VkResult result)
		{
			vk::detail::resultCheck(vk::Result(result), what);
		}
	}
}
