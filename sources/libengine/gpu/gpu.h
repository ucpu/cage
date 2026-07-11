#include <memory>
#include <vector>

#include <svector.h>

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
			struct Frame
			{
				vk::Image image;
				vk::UniqueSemaphore imageAcquiredSemaphore;
				vk::UniqueSemaphore renderCompleteSemaphore;
				vk::UniqueFence fence;
				gpu::Texture texture;
			};

			vkb::Swapchain swapchain;
			std::vector<Frame> frames;
			Vec2i resolution;
			vk::Instance instance;
			vk::SurfaceKHR surface;
			uint32 index = 0;

			WindowGpuContextData(vk::Instance instance);
			~WindowGpuContextData();

			void init(vk::Device device);
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

			PointerRange<char> mappedRange;
			uint64 size = 0;
			gpu::BufferUsageFlags usage = gpu::BufferUsageFlags::Undefined;

			Buffer(const Device &device, const gpu::BufferDescriptor &desc);
			~Buffer();

			void unmap();
		};

		class CommandBuffer : private Immovable
		{
		public:
			std::vector<vk::UniqueCommandBuffer> buffers;

			CommandBuffer(const CommandEncoder &commandEncoder);
			~CommandBuffer();
		};

		class CommandEncoder : private Immovable
		{
		public:
			CommandEncoder(const Device &device, const gpu::CommandEncoderDescriptor &desc);
			~CommandEncoder();

			void copyTextureToBuffer(const gpu::TexelCopyTextureInfo &source, const gpu::TexelCopyBufferInfo &destination, Vec3i copySize);
		};

		class Device : private Immovable
		{
			struct Bootstrap
			{
				vkb::Instance inst;
				vkb::PhysicalDevice phys;
				vkb::Device dev;
				VkQueue q = nullptr;

				~Bootstrap();
			};
			Bootstrap bootstrap;

			std::vector<std::shared_ptr<WindowGpuContextData>> surfacesCollection;

		public:
			vk::Instance instance;
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			vk::Queue queue;
			VmaAllocator allocator = nullptr;
			vk::UniqueDescriptorPool descriptorPool;

			Device(const gpu::GpuDeviceDescriptor &desc);
			~Device();

			void bootstrapInit(const gpu::GpuDeviceDescriptor &desc);
			void tick();
			Holder<privat::WindowGpuContext> getWindowGpuContext(Window *window);
			gpu::Texture acquireWindowSurfaceTexture(Window *window);
			void windowPresent(Window *window);
			void windowWaitFence(Window *window);

			void writeBuffer(const gpu::Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void writeTexture(const gpu::TexelCopyTextureInfo &dest, PointerRange<const char> data, const gpu::TexelCopyBufferLayout &layout, Vec3i extents);
		};

		class RenderPassEncoder : private Immovable
		{
		public:
			RenderPassEncoder(const CommandEncoder &commandEncoder, const gpu::RenderPassDescriptor &desc);
			~RenderPassEncoder();

			void draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance);
			void drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance);
			void endPass();
			void popDebugGroup();
			void pushDebugGroup(gpu::StringView label);
			void setBindGroup(uint32 binding, const gpu::BindGroup &group);
			void setBindGroup(uint32 binding, const gpu::BindGroup &group, PointerRange<const uint32> dynamicOffsets);
			void setIndexBuffer(const gpu::Buffer &buffer, gpu::IndexFormatEnum format, uint64 offset, uint64 size);
			void setPipeline(const gpu::RenderPipeline &pipeline);
			void setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h);
			void setVertexBuffer(uint32 slot, const gpu::Buffer &buffer, uint64 offset, uint64 size);
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

		CAGE_FORCE_INLINE void check(const char *what, vk::Result result)
		{
			check(what, (VkResult)result);
		}

		template<class T, class Src>
		CAGE_FORCE_INLINE vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> makeUnique(Src &src)
		{
			return vk::UniqueHandle<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>(T(src), typename vk::UniqueHandleTraits<T, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>::deleter());
		}

		vk::ShaderStageFlags convertShaderStages(gpu::ShaderStagesFlags visibility);
		vk::PrimitiveTopology convertPrimitiveTopology(gpu::PrimitiveTopologyEnum topology);
		vk::CullModeFlags convertCullMode(gpu::CullModeEnum mode);
		vk::CompareOp convertCompareFunction(gpu::CompareFunctionEnum comp);
		vk::VertexInputRate convertVertexStepMode(gpu::VertexStepModeEnum mode);
		vk::Format convertVertexFormat(gpu::VertexFormatEnum format);
		vk::BlendFactor convertBlendFactor(gpu::BlendFactorEnum factor);
		vk::BlendOp convertBlendOperation(gpu::BlendOperationEnum op);
		vk::Format convertTextureFormat(gpu::TextureFormatEnum format);
	}
}
