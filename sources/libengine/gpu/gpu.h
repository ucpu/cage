#include <memory>
#include <vector>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cage-engine/gpuInterface.h>

namespace cage
{
	namespace gpuImpl
	{
		struct WindowGpuContextData;
	}

	namespace privat
	{
		struct WindowGpuContext : private Immovable
		{
			// the window owns its surface, but the device can destroy all the surfaces in its destructor
			std::shared_ptr<gpuImpl::WindowGpuContextData> data;
		};
	}

	namespace gpuImpl
	{
		struct WindowGpuContextData : private Immovable
		{
			vkb::Swapchain swapchain;
			std::vector<vk::Image> images;
			std::vector<vk::ImageView> views;
			Vec2i resolution;
			vk::Instance instance;
			vk::SurfaceKHR surface;
			vk::Semaphore imageAvailable;
			vk::Semaphore renderFinished;
			vk::Fence inFlight;

			WindowGpuContextData(vk::Instance instance);
			~WindowGpuContextData();

			void init();
			void clear();
		};

		class BindGroup : private Immovable
		{
		public:
			vk::UniqueDescriptorSet set;

			BindGroup(const Device &device, const gpu::BindGroupDescriptor &desc);
			~BindGroup();
		};

		class BindGroupLayout : private Immovable
		{
		public:
			vk::UniqueDescriptorSetLayout layout;

			BindGroupLayout(const Device &device, const gpu::BindGroupLayoutDescriptor &desc);
			~BindGroupLayout();
		};

		class Buffer : private Immovable
		{
		public:
			vk::UniqueBuffer buffer;

			uint64 size = 0;
			gpu::BufferUsageFlags usage = gpu::BufferUsageFlags::Undefined;

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

			std::vector<std::shared_ptr<WindowGpuContextData>> surfacesCollection;

		public:
			vk::Instance instance;
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			vk::Queue queueTransfer;
			vk::Queue queueGraphics;
			vk::Queue queuePresent;
			VmaAllocator allocator = nullptr;
			vk::UniqueDescriptorPool descriptorPool;

			Device(const gpu::GpuDeviceDescriptor &desc);
			~Device();

			void bootstrapInit(const gpu::GpuDeviceDescriptor &desc);
			Holder<privat::WindowGpuContext> getWindowGpuContext(Window *window);
			gpu::Texture getWindowSurfaceTexture(Window *window);
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
			vk::UniquePipeline pipeline;

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
			vk::UniquePipelineLayout layout;

			PipelineLayout(const Device &device, const gpu::PipelineLayoutDescriptor &desc);
			~PipelineLayout();
		};

		class Texture : private Immovable
		{
		public:
			vk::UniqueImage image;

			Vec3i resolution;
			uint32 mipLevels = 0;
			gpu::TextureDimensionEnum dimension = gpu::TextureDimensionEnum::Undefined;
			gpu::TextureFormatEnum format = gpu::TextureFormatEnum::Undefined;

			Texture(const Device &device, const gpu::TextureDescriptor &desc);
			~Texture();
		};

		class TextureView : private Immovable
		{
		public:
			vk::UniqueImageView view;

			TextureView(const Texture &texture, const gpu::TextureViewDescriptor &desc);
			~TextureView();
		};

		CAGE_FORCE_INLINE void check(const char *what, VkResult result)
		{
			vk::detail::resultCheck(vk::Result(result), what);
		}

		template<class T, class Src>
		CAGE_FORCE_INLINE vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> makeUnique(Src &src)
		{
			return vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>(T(src), typename vk::UniqueHandleTraits<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::deleter());
		}
	}
}
