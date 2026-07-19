#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

#include <svector.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <cage-core/concurrent.h>
#include <cage-core/tasks.h>
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
		///////////////////////////////////////////////////////////////////
		// implementation utilities
		///////////////////////////////////////////////////////////////////

		enum class ImageStateEnum
		{
			Undefined = 0,
			ColorAttachment,
			DepthAttachment,
			Sampled,
			StorageRead,
			StorageWrite,
			TransferSrc,
			TransferDst,
			Present,
		};

		struct Hasher
		{
			auto operator()(const Texture &t) const noexcept { return std::hash<void *>()(t.get()); }
			auto operator()(const Texture &a, const Texture &b) const noexcept { return a.get() == b.get(); }
		};

		struct Nothing
		{};

		struct DeferredDestruction : private Noncopyable
		{
			using Destructor = void (*)(void *ptr);
			alignas(8) char data[40] = {};
			Destructor destructor = nullptr;

			DeferredDestruction() {}
			DeferredDestruction(DeferredDestruction &&other) noexcept
			{
				detail::memcpy(this, &other, sizeof(*this));
				other.destructor = nullptr;
			}
			DeferredDestruction &operator=(DeferredDestruction &&other) noexcept
			{
				if (this == &other)
					return *this;
				if (destructor)
					destructor(data);
				detail::memcpy(this, &other, sizeof(*this));
				other.destructor = nullptr;
			}
			~DeferredDestruction()
			{
				if (destructor)
					destructor(data);
			}
		};

		template<class T, class Extra = Nothing>
		struct ResourceHandle : private Noncopyable
		{
			DeviceImpl *device = nullptr;
			T value = {};
			[[no_unique_address]] Extra extra = {};

			ResourceHandle(DeviceImpl &device) : device(&device) {}
			ResourceHandle(ResourceHandle &&other) noexcept { *this = other; }
			~ResourceHandle() { destroy(); }
			ResourceHandle &operator=(ResourceHandle &&other) noexcept
			{
				if (this == &other)
					return *this;
				destroy();
				device = nullptr;
				value = {}; // make sure the moved-from object becomes empty
				extra = {};
				std::swap(device, other.device);
				std::swap(value, other.value);
				std::swap(extra, other.extra);
				return *this;
			}

			operator T &() { return value; }
			T *operator->() { return &value; }

			void setLabel(StringView label);
			void destroy();
		};

		template<class T, class Extra>
		void deferredDestructor(ResourceHandle<T, Extra> &);

		struct ResourcesToKeepAlive : private Noncopyable
		{
			ResourceHandle<std::vector<Holder<void>>> resourcesToKeepAlive;

			ResourcesToKeepAlive(DeviceImpl &device) : resourcesToKeepAlive(device) {}

			template<class Crtp, class Impl>
			void keepAlive(const GpuInterfaceHandle<Crtp, Impl> &v)
			{
				resourcesToKeepAlive.value.push_back(v.getVoidHolder());
			}
		};

		struct WindowGpuContextImpl : private Immovable
		{
			struct SwpImage
			{
				Texture texture;
				vk::Image image;
				vk::UniqueSemaphore renderComplete;
			};
			struct FrameInFlight
			{
				vk::UniqueSemaphore imageAcquired;
				vk::Fence fence;
			};

			vkb::Swapchain swapchain;
			std::vector<SwpImage> swpImages;
			std::array<FrameInFlight, 2> framesInFlight = {};
			Vec2i resolution;
			vk::Instance instance;
			vk::SurfaceKHR surface;
			uint32 imageIndex = m;
			uint32 frameIndex = 0;

			WindowGpuContextImpl(vk::Instance instance);
			~WindowGpuContextImpl();

			void init(DeviceImpl &device);
			void clear();

			SwpImage &img()
			{
				CAGE_ASSERT(imageIndex < swpImages.size());
				return swpImages[imageIndex];
			}
			FrameInFlight &frm()
			{
				CAGE_ASSERT(frameIndex < framesInFlight.size());
				return framesInFlight[frameIndex];
			}
		};

		///////////////////////////////////////////////////////////////////
		// classes implementation
		///////////////////////////////////////////////////////////////////

		class BindGroupImpl : private Immovable
		{
		public:
			ResourceHandle<vk::DescriptorSet, vk::DescriptorPool> set;
			ResourcesToKeepAlive rtka;

			BindGroupImpl(DeviceImpl &device, const BindGroupDescriptor &desc);
			~BindGroupImpl();
		};

		class BindGroupLayoutImpl : private Immovable
		{
		public:
			ResourceHandle<vk::DescriptorSetLayout> layout;
			ankerl::svector<BindGroupLayoutDescriptor::Entry, 10> entries;

			BindGroupLayoutImpl(DeviceImpl &device, const BindGroupLayoutDescriptor &desc);
			~BindGroupLayoutImpl();
		};

		class BufferImpl : private Immovable
		{
		public:
			ResourceHandle<vk::Buffer, VmaAllocation> buffer;
			VmaAllocationInfo allocatedInfo = {};

			PointerRange<char> mappedRange;
			uint64 size = 0;
			BufferUsageFlags usage = BufferUsageFlags::Undefined;

			BufferImpl(DeviceImpl &device, const BufferDescriptor &desc);
			~BufferImpl();

			void flush();
			void invalidate();
		};

		class CommandBufferImpl : private Immovable
		{
		public:
			ResourceHandle<vk::CommandPool> pool;
			ResourceHandle<vk::CommandBuffer, vk::CommandPool> buffer;
			ResourcesToKeepAlive rtka;

			CommandBufferImpl(DeviceImpl &device);
			~CommandBufferImpl();
		};

		class CommandEncoderImpl : private Immovable
		{
		public:
			CommandBuffer buffer;
			std::unordered_map<Texture, ImageStateEnum, Hasher, Hasher> transitionedImages;
			vk::CommandBuffer cmd;
			vk::PipelineLayout currentPipelineLayout;
			EncoderModeEnum currentMode = EncoderModeEnum::Generic;

			CommandEncoderImpl(DeviceImpl &device, const CommandEncoderDescriptor &desc);
			~CommandEncoderImpl();

			template<class Crtp, class Impl>
			void keepAlive(const GpuInterfaceHandle<Crtp, Impl> &v)
			{
				buffer->rtka.keepAlive(v);
			}

			// records a barrier with layout transition (if needed)
			// permanent = false: keeps track of the changed state to automatically revert it to the default layout at the end
			// permanent = true: updates the default layout of the image - the image must not be used concurrently in any other command encoders - used for initialization only
			void imageTransition(const Texture &texture, ImageStateEnum targetState, bool permanent = false);

			// encoder

			void popDebugGroup();
			void pushDebugGroup(StringView label);
			CommandBuffer finishEncoding();

			// generic encoder

			void copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize);

			// render pass ecnoder

			void beginRenderPass(const RenderPassDescriptor &descriptor);
			void endRenderPass();
			void draw(uint32 verticesCount, uint32 instancesCount, uint32 firstVertex, uint32 firstInstance);
			void drawIndexed(uint32 indicesCount, uint32 instancesCount, uint32 firstIndex, sint32 baseVertex, uint32 firstInstance);
			void setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets);
			void setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset, uint64 size);
			void setPipeline(const RenderPipeline &pipeline);
			void setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h);
			void setVertexBuffer(uint32 slot, const Buffer &buffer, uint64 offset, uint64 size);
			void setViewport(Real x, Real y, Real width, Real height, Real minDepth, Real maxDepth);

			// compute pass encoder
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

		public:
			Holder<Mutex> mutex = newMutex();
			vk::Instance instance;
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			vk::Queue queue;
			VmaAllocator allocator = nullptr;
			vk::UniqueDescriptorPool descriptorPool;
			std::vector<CommandBuffer> additionalCommands;
			std::vector<std::shared_ptr<WindowGpuContextImpl>> surfacesCollection;
			std::array<vk::UniqueFence, 2> framesFences = {};
			std::array<std::vector<DeferredDestruction>, 3> deferredDestructions = {}; // insert into [0]
			std::vector<Holder<AsyncTask>> disposingTasks;

			DeviceImpl(const GpuDeviceDescriptor &desc);
			~DeviceImpl();

			template<class T>
			void setLabel(const T &object, StringView label);

			void bootstrapInit(const GpuDeviceDescriptor &desc);
			Holder<privat::WindowGpuContext> getWindowGpuContext(Window *window);

			void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents);

			void submitAndPresentWindows(PointerRange<const CommandBuffer> buffers, PointerRange<WindowPresentationDescriptor> windows);
		};

		class RenderPipelineImpl : private Immovable
		{
		public:
			ResourceHandle<vk::Pipeline> pipeline;
			PipelineLayout layout;

			RenderPipelineImpl(DeviceImpl &device, const RenderPipelineDescriptor &desc);
			~RenderPipelineImpl();
		};

		class SamplerImpl : private Immovable
		{
		public:
			ResourceHandle<vk::Sampler> sampler;

			SamplerImpl(DeviceImpl &device, const SamplerDescriptor &desc);
			~SamplerImpl();
		};

		class ShaderModuleImpl : private Immovable
		{
		public:
			ResourceHandle<vk::ShaderModule> shader;

			ShaderModuleImpl(DeviceImpl &device, const ShaderModuleDescriptor &desc);
			~ShaderModuleImpl();
		};

		class PipelineLayoutImpl : private Immovable
		{
		public:
			ResourceHandle<vk::PipelineLayout> layout;

			PipelineLayoutImpl(DeviceImpl &device, const PipelineLayoutDescriptor &desc);
			~PipelineLayoutImpl();
		};

		class TextureImpl : private Immovable
		{
		public:
			ResourceHandle<vk::Image, VmaAllocation> image;
			VmaAllocationInfo allocatedInfo = {};

			Vec3i resolution;
			uint32 arrayLayers = 0;
			uint32 mipLevels = 0;
			TextureDimensionEnum dimension = TextureDimensionEnum::Undefined;
			TextureFormatEnum format = TextureFormatEnum::Undefined;
			TextureUsageFlags usage = TextureUsageFlags::Undefined;
			ImageStateEnum defaultState = ImageStateEnum::Undefined;

			TextureImpl(DeviceImpl &device, vk::Image image);
			TextureImpl(DeviceImpl &device, const TextureDescriptor &desc);
			~TextureImpl();
		};

		class TextureViewImpl : private Immovable
		{
		public:
			ResourceHandle<vk::ImageView> view;
			Texture texture; // ensure the texture outlives the view

			TextureViewImpl(const Texture &texture, const TextureViewDescriptor &desc);
			~TextureViewImpl();
		};

		///////////////////////////////////////////////////////////////////
		// extra functions
		///////////////////////////////////////////////////////////////////

		CAGE_FORCE_INLINE void check(const char *what, VkResult result)
		{
			vk::detail::resultCheck(vk::Result(result), what);
		}

		CAGE_FORCE_INLINE void check(const char *what, vk::Result result)
		{
			check(what, (VkResult)result);
		}

		vk::AttachmentLoadOp convertLoadOperation(LoadOpEnum op);
		vk::AttachmentStoreOp convertStoreOperation(StoreOpEnum op);
		vk::BlendFactor convertBlendFactor(BlendFactorEnum factor);
		vk::BlendOp convertBlendOperation(BlendOperationEnum op);
		vk::BufferUsageFlags convertBufferUsage(BufferUsageFlags flags);
		vk::CompareOp convertCompareFunction(CompareFunctionEnum comp);
		vk::CullModeFlags convertCullMode(CullModeEnum mode);
		vk::Filter convertFilter(FilterModeEnum filter);
		vk::Format convertTextureFormat(TextureFormatEnum format);
		vk::Format convertVertexFormat(VertexFormatEnum format);
		vk::FrontFace convertFrontFace(FrontFaceEnum face);
		vk::ImageAspectFlags convertAspectMask(TextureFormatEnum format);
		vk::ImageUsageFlags convertTextureUsage(TextureUsageFlags flags, TextureFormatEnum format);
		vk::IndexType convertIndexFormat(IndexFormatEnum format);
		vk::PrimitiveTopology convertPrimitiveTopology(PrimitiveTopologyEnum topology);
		vk::SamplerAddressMode convertAddressMode(AddressModeEnum mode);
		vk::SamplerMipmapMode convertMipmapFilter(FilterModeEnum filter);
		vk::ShaderStageFlags convertShaderStages(ShaderStagesFlags visibility);
		vk::DescriptorType convertBindingBufferType(BufferBindingTypeEnum type, bool hasDynamicOffset);
		void assignImageStateBarrierFlags(ImageStateEnum state, vk::PipelineStageFlags2 &stageMask, vk::AccessFlags2 &accessMask, vk::ImageLayout &imageLayout);

		///////////////////////////////////////////////////////////////////
		// inline implementations
		///////////////////////////////////////////////////////////////////

		template<class T, class Extra>
		void ResourceHandle<T, Extra>::destroy()
		{
			static_assert(sizeof(*this) <= sizeof(DeferredDestruction::data));
			if constexpr (requires() { !value; })
			{
				if (!value)
					return;
			}
			if constexpr (requires() { value.empty(); })
			{
				if (value.empty())
					return;
			}
			CAGE_ASSERT(device);
			DeferredDestruction dd;
			detail::memcpy(dd.data, this, sizeof(*this));
			dd.destructor = +[](void *ptr)
			{
				ResourceHandle<T, Extra> h(*(DeviceImpl *)nullptr);
				detail::memcpy(&h, ptr, sizeof(h));
				deferredDestructor(h);
				detail::memset(&h, 0, sizeof(h));
			};
			device->deferredDestructions[0].push_back(std::move(dd));
			detail::memset(this, 0, sizeof(*this));
		}

		template<class T, class Extra>
		void ResourceHandle<T, Extra>::setLabel(StringView label)
		{
			device->setLabel(value, label);
		}

		template<class T>
		void DeviceImpl::setLabel(const T &object, StringView label)
		{
			if (label.str.empty())
				return;
			vk::DebugUtilsObjectNameInfoEXT info;
			if constexpr (requires { object->objectType; })
			{
				info.objectType = object->objectType;
				info.objectHandle = (uint64) static_cast<const typename T::element_type::CType>(*object);
			}
			else if constexpr (requires { object.objectType; })
			{
				info.objectType = object.objectType;
				info.objectHandle = (uint64) static_cast<const typename T::CType>(object);
			}
			else
			{
				static_assert([] { return false; }(), "unknown vulkan object type");
			}
			info.pObjectName = label.str.data();
			device.setDebugUtilsObjectNameEXT(info);
		}
	}
}
