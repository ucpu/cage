#include <memory>
#include <vector>

#include <svector.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cage-engine/gpuInterface.h>

namespace cage
{
	namespace gpu
	{
		struct WindowGpuContextImpl;
	}

	namespace privat
	{
		struct WindowGpuContext : private Immovable
		{
			// the window owns its surface, but the device can destroy all the surfaces in its destructor
			std::shared_ptr<gpu::WindowGpuContextImpl> data;
		};
	}

	namespace gpu
	{
		struct WindowGpuContextImpl : private Immovable
		{
			struct Frame
			{
				vk::Image image;
				vk::UniqueSemaphore imageAcquiredSemaphore;
				vk::UniqueSemaphore renderCompleteSemaphore;
				vk::UniqueFence fence;
				Texture texture;
			};

			vkb::Swapchain swapchain;
			std::vector<Frame> frames;
			Vec2i resolution;
			vk::Instance instance;
			vk::SurfaceKHR surface;
			uint32 index = 0;

			WindowGpuContextImpl(vk::Instance instance);
			~WindowGpuContextImpl();

			void init(vk::Device device);
			void clear();
		};

		class BindGroupImpl : private Immovable
		{
		public:
			vk::UniqueDescriptorSet set;

			BindGroupImpl(const DeviceImpl &device, const BindGroupDescriptor &desc);
			~BindGroupImpl();
		};

		class BindGroupLayoutImpl : private Immovable
		{
		public:
			vk::UniqueDescriptorSetLayout layout;

			BindGroupLayoutImpl(const DeviceImpl &device, const BindGroupLayoutDescriptor &desc);
			~BindGroupLayoutImpl();
		};

		class BufferImpl : private Immovable
		{
		public:
			const DeviceImpl *device = nullptr;
			vk::Buffer buffer;
			VmaAllocationInfo allocatedInfo;
			VmaAllocation allocation = nullptr;

			PointerRange<char> mappedRange;
			uint64 size = 0;
			BufferUsageFlags usage = BufferUsageFlags::Undefined;

			BufferImpl(const DeviceImpl &device, const BufferDescriptor &desc);
			~BufferImpl();

			void flush();
			void invalidate();
		};

		class CommandBufferImpl : private Immovable
		{
		public:
			std::vector<vk::UniqueCommandBuffer> buffers;
			vk::Device device;

			CommandBufferImpl(vk::Device device);
			~CommandBufferImpl();

			vk::UniqueCommandBuffer newBuffer();
		};

		class CommandEncoderImpl : private Immovable
		{
		public:
			CommandBuffer buffer;
			vk::UniqueCommandBuffer cmd;

			CommandEncoderImpl(const DeviceImpl &device, const CommandEncoderDescriptor &desc);
			~CommandEncoderImpl();

			void copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize);

			CommandBuffer finishEncoding();
		};

		class DeviceImpl : private Immovable
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

			std::vector<std::shared_ptr<WindowGpuContextImpl>> surfacesCollection;

		public:
			vk::Instance instance;
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			vk::Queue queue;
			VmaAllocator allocator = nullptr;
			vk::UniqueDescriptorPool descriptorPool;

			DeviceImpl(const GpuDeviceDescriptor &desc);
			~DeviceImpl();

			void bootstrapInit(const GpuDeviceDescriptor &desc);
			void tick();
			Holder<privat::WindowGpuContext> getWindowGpuContext(Window *window);
			Texture acquireWindowSurfaceTexture(Window *window);
			void windowPresent(Window *window);
			void windowWaitFence(Window *window);

			void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents);
		};

		class RenderPassEncoderImpl : private Immovable
		{
		public:
			CommandEncoder encoder;
			vk::PipelineLayout layout;

			RenderPassEncoderImpl(const CommandEncoder &commandEncoder, const RenderPassDescriptor &desc);
			~RenderPassEncoderImpl();

			void draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance);
			void drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance);
			void endPass();
			void popDebugGroup();
			void pushDebugGroup(StringView label);
			void setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets);
			void setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size);
			void setPipeline(const RenderPipeline &pipeline);
			void setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h);
			void setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size);
		};

		class RenderPipelineImpl : private Immovable
		{
		public:
			vk::UniquePipeline pipeline;
			PipelineLayout layout;

			RenderPipelineImpl(const DeviceImpl &device, const RenderPipelineDescriptor &desc);
			~RenderPipelineImpl();
		};

		class SamplerImpl : private Immovable
		{
		public:
			vk::UniqueSampler sampler;

			SamplerImpl(const DeviceImpl &device, const SamplerDescriptor &desc);
			~SamplerImpl();
		};

		class ShaderModuleImpl : private Immovable
		{
		public:
			vk::UniqueShaderModule shader;

			ShaderModuleImpl(const DeviceImpl &device, const ShaderModuleDescriptor &desc);
			~ShaderModuleImpl();
		};

		class PipelineLayoutImpl : private Immovable
		{
		public:
			vk::UniquePipelineLayout layout;

			PipelineLayoutImpl(const DeviceImpl &device, const PipelineLayoutDescriptor &desc);
			~PipelineLayoutImpl();
		};

		class TextureImpl : private Immovable
		{
		public:
			vk::UniqueImage image;

			Vec3i resolution;
			uint32 mipLevels = 0;
			TextureDimensionEnum dimension = TextureDimensionEnum::Undefined;
			TextureFormatEnum format = TextureFormatEnum::Undefined;

			TextureImpl(const DeviceImpl &device, const TextureDescriptor &desc);
			~TextureImpl();
		};

		class TextureViewImpl : private Immovable
		{
		public:
			vk::UniqueImageView view;

			TextureViewImpl(const TextureImpl &texture, const TextureViewDescriptor &desc);
			~TextureViewImpl();
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

		vk::ShaderStageFlags convertShaderStages(ShaderStagesFlags visibility);
		vk::PrimitiveTopology convertPrimitiveTopology(PrimitiveTopologyEnum topology);
		vk::CullModeFlags convertCullMode(CullModeEnum mode);
		vk::CompareOp convertCompareFunction(CompareFunctionEnum comp);
		vk::VertexInputRate convertVertexStepMode(VertexStepModeEnum mode);
		vk::Format convertVertexFormat(VertexFormatEnum format);
		vk::BlendFactor convertBlendFactor(BlendFactorEnum factor);
		vk::BlendOp convertBlendOperation(BlendOperationEnum op);
		vk::Format convertTextureFormat(TextureFormatEnum format);
		vk::IndexType convertIndexFormat(IndexFormatEnum format);
		vk::AttachmentLoadOp convertLoadOperation(LoadOpEnum op);
		vk::AttachmentStoreOp convertStoreOperation(StoreOpEnum op);
		vk::BufferUsageFlagBits convertBufferUsage(BufferUsageFlags flags);
	}
}
