#ifndef guard_gpuInterface_sdrzuij4rt5e
#define guard_gpuInterface_sdrzuij4rt5e

#include <optional>
#include <variant>

#include <cage-engine/gpuCore.h>

namespace cage
{
	class Window;

	namespace gpuImpl
	{
		class BindGroup;
		class BindGroupLayout;
		class Buffer;
		class CommandBuffer;
		class CommandEncoder;
		class Device;
		class RenderPassEncoder;
		class RenderPipeline;
		class Sampler;
		class ShaderModule;
		class PipelineLayout;
		class Texture;
		class TextureView;
	}

	namespace gpu
	{
		///////////////////////////////////////////////////////////////////
		// basic primitives
		///////////////////////////////////////////////////////////////////

		struct CAGE_ENGINE_API StringView
		{
			PointerRange<const char> str;

			StringView() {}
			StringView(const char *ptr);
			StringView(const String &ptr) : str(ptr) {}
			StringView(const AssetLabel &ptr) : str(ptr) {}
		};

		struct CAGE_ENGINE_API Future
		{};

		///////////////////////////////////////////////////////////////////
		// gpu resource handles
		///////////////////////////////////////////////////////////////////

		template<class Crtp, class Impl>
		class GpuResourceHandle
		{
		public:
			CAGE_FORCE_INLINE GpuResourceHandle() noexcept {}
			CAGE_FORCE_INLINE GpuResourceHandle(Holder<Impl> &&impl) : ptr(std::move(impl)) {}
			CAGE_FORCE_INLINE GpuResourceHandle(const GpuResourceHandle &other) : ptr(other.ptr.share()) {}
			CAGE_FORCE_INLINE GpuResourceHandle(GpuResourceHandle &&other) noexcept : ptr(std::move(other.ptr)) {}
			CAGE_FORCE_INLINE GpuResourceHandle &operator=(const GpuResourceHandle &other)
			{
				ptr = other.ptr.share();
				return *this;
			}
			CAGE_FORCE_INLINE GpuResourceHandle &operator=(GpuResourceHandle &&other) noexcept
			{
				ptr = std::move(other.ptr);
				return *this;
			}

			CAGE_FORCE_INLINE explicit operator bool() const noexcept { return !!ptr; }
			CAGE_FORCE_INLINE Impl *get() const
			{
				CAGE_ASSERT(ptr);
				return +ptr;
			}
			CAGE_FORCE_INLINE Impl *operator->() const
			{
				CAGE_ASSERT(ptr);
				return +ptr;
			}

		private:
			Holder<Impl> ptr;
			friend Crtp;
		};

		class CAGE_ENGINE_API BindGroup : public GpuResourceHandle<BindGroup, gpuImpl::BindGroup>
		{
		public:
		};

		class CAGE_ENGINE_API BindGroupLayout : public GpuResourceHandle<BindGroupLayout, gpuImpl::BindGroupLayout>
		{
		public:
		};

		class CAGE_ENGINE_API Buffer : public GpuResourceHandle<Buffer, gpuImpl::Buffer>
		{
		public:
			uint64 getSize() const;
			BufferUsageFlags getUsage() const;
			//BufferMapState getMapState() const;
			PointerRange<char> getMappedRange();
			template<class Callable>
			Future mapAsync(MapModeEnum mapMode, uint64 offset, uint64 size, CallbackModeEnum callbackMode, Callable callable);
			void unmap();
		};

		class CAGE_ENGINE_API CommandBuffer : public GpuResourceHandle<CommandBuffer, gpuImpl::CommandBuffer>
		{
		public:
		};

		class CAGE_ENGINE_API CommandEncoder : public GpuResourceHandle<CommandEncoder, gpuImpl::CommandEncoder>
		{
		public:
			RenderPassEncoder beginRenderPass(const RenderPassDescriptor &descriptor);

			//void clearBuffer(const Buffer &buffer, uint64 offset = 0, uint64 size = m);
			//void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);

			//void copyBufferToBuffer(const Buffer &source, uint64 sourceOffset, const Buffer &destination, uint64 destinationOffset, uint64 size);
			//void copyBufferToTexture(const TexelCopyBufferInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize);
			void copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize);
			//void copyTextureToTexture(const TexelCopyTextureInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize);

			//void resolveQuerySet(const QuerySet &querySet, uint32 firstQuery, uint32 queryCount, const Buffer &destination, uint64 destinationOffset);
			//void writeTimestamp(const QuerySet &querySet, uint32 queryIndex);

			CommandBuffer finishEncoding();
		};

		class CAGE_ENGINE_API Device : public GpuResourceHandle<Device, gpuImpl::Device>
		{
		public:
			BindGroup createBindGroup(const BindGroupDescriptor &descriptor);
			BindGroupLayout createBindGroupLayout(const BindGroupLayoutDescriptor &descriptor);
			Buffer createBuffer(const BufferDescriptor &descriptor);
			CommandEncoder createCommandEncoder(const CommandEncoderDescriptor &descriptor);
			PipelineLayout createPipelineLayout(const PipelineLayoutDescriptor &descriptor);
			//QuerySet createQuerySet(const QuerySetDescriptor &descriptor);
			RenderPipeline createRenderPipeline(const RenderPipelineDescriptor &descriptor);
			template<class Callable>
			Future createRenderPipelineAsync(const RenderPipelineDescriptor &descriptor, CallbackModeEnum callbackMode, Callable callable);
			Sampler createSampler(const SamplerDescriptor &descriptor);
			ShaderModule createShaderModule(const ShaderModuleDescriptor &descriptor);
			Texture createTexture(const TextureDescriptor &descriptor);

			template<class Callable>
			Future onSubmittedWorkDone(CallbackModeEnum callbackMode, Callable callable);
			//void submit(PointerRange<const CommandBuffer> commands);
			void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents);

			void tick();
			Texture windowAcquireTexture(Window *window);
			void windowPresent(Window *window);
			void windowWaitFence(Window *window);
		};

		class CAGE_ENGINE_API RenderPassEncoder : public GpuResourceHandle<RenderPassEncoder, gpuImpl::RenderPassEncoder>
		{
		public:
			void draw(uint32 verticesCount, uint32 instancesCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0);
			void drawIndexed(uint32 indicesCount, uint32 instancesCount = 1, uint32 firstIndex = 0, sint32 baseVertex = 0, uint32 firstInstance = 0);
			//void drawIndexedIndirect(const Buffer &indirectBuffer, uint64 indirectOffset);
			//void drawIndirect(const Buffer &indirectBuffer, uint64 indirectOffset);
			void endPass();
			//void multiDrawIndexedIndirect(const Buffer &indirectBuffer, uint64 indirectOffset, uint32 maxDrawCount, const Buffer &drawCountBuffer = {}, uint64 drawCountBufferOffset = 0);
			//void multiDrawIndirect(const Buffer &indirectBuffer, uint64 indirectOffset, uint32 maxDrawCount, const Buffer &drawCountBuffer = {}, uint64 drawCountBufferOffset = 0);
			//void pixelLocalStorageBarrier();
			void popDebugGroup();
			void pushDebugGroup(StringView label);
			void setBindGroup(uint32 binding, const BindGroup &group = {});
			void setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets);
			//void setBlendConstant(Vec4 color);
			//void setImmediates(uint32 offset, PointerRange<const char> data);
			void setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset = 0, uint64 size = m);
			void setPipeline(const RenderPipeline &pipeline);
			void setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h);
			//void setStencilReference(uint32 reference);
			void setVertexBuffer(uint32 slot, const Buffer &buffer = {}, uint64 offset = 0, uint64 size = m);
			//void setViewport(Real x, Real y, Real width, Real height, Real minDepth, Real maxDepth);
			//void writeTimestamp(const QuerySet &querySet, uint32 queryIndex);
		};

		class CAGE_ENGINE_API RenderPipeline : public GpuResourceHandle<RenderPipeline, gpuImpl::RenderPipeline>
		{
		public:
			//BindGroupLayout getBindGroupLayout(uint32 groupIndex);
		};

		class CAGE_ENGINE_API Sampler : public GpuResourceHandle<Sampler, gpuImpl::Sampler>
		{
		public:
		};

		class CAGE_ENGINE_API ShaderModule : public GpuResourceHandle<ShaderModule, gpuImpl::ShaderModule>
		{
		public:
		};

		class CAGE_ENGINE_API PipelineLayout : public GpuResourceHandle<PipelineLayout, gpuImpl::PipelineLayout>
		{
		public:
		};

		class CAGE_ENGINE_API Texture : public GpuResourceHandle<Texture, gpuImpl::Texture>
		{
		public:
			TextureView createView(const TextureViewDescriptor &desc);
			uint32 getDepthOrArrayLayers() const;
			TextureDimensionEnum getDimension() const;
			TextureFormatEnum getFormat() const;
			uint32 getHeight() const;
			uint32 getMipLevelCount() const;
			//uint32 getSampleCount() const;
			//TextureViewDimension getTextureBindingViewDimension() const;
			//TextureUsageFlags getUsage() const;
			uint32 getWidth() const;
			//void pin(TextureUsageFlags usage);
			//void unpin();
		};

		class CAGE_ENGINE_API TextureView : public GpuResourceHandle<TextureView, gpuImpl::TextureView>
		{
		public:
		};

		///////////////////////////////////////////////////////////////////
		// descriptors
		///////////////////////////////////////////////////////////////////

		struct CAGE_ENGINE_API BindGroupDescriptor
		{
			StringView label;

			struct BufferEntry
			{
				Buffer buffer;
				uint64 offset = 0;
				uint64 size = 0; // use -1 to use the whole buffer
			};
			struct SamplerEntry
			{
				Sampler sampler;
			};
			struct TextureEntry
			{
				TextureView textureView;
			};

			struct Entry
			{
				std::variant<std::monostate, BufferEntry, SamplerEntry, TextureEntry> data;
				uint32 binding = 0;
			};
			PointerRange<const Entry> entries;

			BindGroupLayout layout;
		};

		struct CAGE_ENGINE_API BindGroupLayoutDescriptor
		{
			StringView label;

			struct BufferEntry
			{
				BufferBindingTypeEnum type = BufferBindingTypeEnum::Undefined;
				bool hasDynamicOffset = false;
			};
			struct SamplerEntry
			{
				SamplerBindingTypeEnum type = SamplerBindingTypeEnum::Undefined;
			};
			struct TextureEntry
			{
				TextureSampleTypeEnum sampleType = TextureSampleTypeEnum::Undefined;
				TextureDimensionEnum viewDimension = TextureDimensionEnum::Undefined;
				//bool multisampled = false;
			};

			struct Entry
			{
				std::variant<std::monostate, BufferEntry, SamplerEntry, TextureEntry> data;
				uint32 binding = 0;
				ShaderStagesFlags visibility = ShaderStagesFlags::Undefined;
			};
			PointerRange<const Entry> entries;
		};

		struct CAGE_ENGINE_API BufferDescriptor
		{
			StringView label;
			uint64 size = 0;
			BufferUsageFlags usage = BufferUsageFlags::Undefined;
			//bool mappedAtCreation = false;
		};

		struct CAGE_ENGINE_API CommandEncoderDescriptor
		{
			StringView label;
		};

		struct CAGE_ENGINE_API PipelineLayoutDescriptor
		{
			PointerRange<const BindGroupLayout> bindGroupLayouts;
			//uint32 immediateSize = 0;
		};

		struct CAGE_ENGINE_API RenderPassDescriptor
		{
			StringView label;

			struct ColorAttachment
			{
				TextureView view;
				//TextureView resolveTarget;
				Vec4 clearValue;
				//uint32 depthSlice = m;
				LoadOpEnum loadOp = LoadOpEnum::Undefined;
				StoreOpEnum storeOp = StoreOpEnum::Undefined;
			};
			PointerRange<const ColorAttachment> colorAttachments;

			struct DepthStencilAttachment
			{
				TextureView view;
				Real depthClearValue;
				//uint32 stencilClearValue = 0;
				LoadOpEnum depthLoadOp = LoadOpEnum::Undefined;
				StoreOpEnum depthStoreOp = StoreOpEnum::Undefined;
				//LoadOpEnum stencilLoadOp = LoadOpEnum::Undefined;
				//StoreOpEnum stencilStoreOp = StoreOpEnum::Undefined;
				//bool depthReadOnly = false;
				//bool stencilReadOnly = false;
			};
			std::optional<DepthStencilAttachment> depthStencilAttachment;
		};

		struct CAGE_ENGINE_API RenderPipelineDescriptor
		{
			PipelineLayout layout;

			struct VertexState
			{
				ShaderModule module;
				PointerRange<const VertexBufferLayout> buffers;
			};
			VertexState vertex;

			struct PrimitiveState
			{
				PrimitiveTopologyEnum topology = PrimitiveTopologyEnum::Undefined;
				//IndexFormatEnum stripIndexFormat = IndexFormatEnum::Undefined;
				//FrontFace frontFace = FrontFace::Undefined;
				CullModeEnum cullMode = CullModeEnum::Undefined;
				//bool unclippedDepth = false;
			};
			PrimitiveState primitive;

			struct DepthStencilState
			{
				//StencilFaceState stencilFront;
				//StencilFaceState stencilBack;
				//uint32 stencilReadMask = 0xFFFFFFFF;
				//uint32 stencilWriteMask = 0xFFFFFFFF;
				//sint32 depthBias = 0;
				//Real depthBiasSlopeScale = 0;
				//Real depthBiasClamp = 0;
				TextureFormatEnum format = TextureFormatEnum::Undefined;
				CompareFunctionEnum depthCompare = CompareFunctionEnum::Undefined;
				bool depthWriteEnabled = false;
			};
			std::optional<DepthStencilState> depthStencil;

			//struct MultisampleState
			//{
			//	uint32 count = 1;
			//	uint32 mask = 0xFFFFFFFF;
			//	bool alphaToCoverageEnabled = false;
			//};
			//std::optional<MultisampleState> multisample;

			struct BlendState
			{
				struct BlendEntry
				{
					BlendOperationEnum operation = BlendOperationEnum::Undefined;
					BlendFactorEnum srcFactor = BlendFactorEnum::Undefined;
					BlendFactorEnum dstFactor = BlendFactorEnum::Undefined;
				};
				BlendEntry color;
				BlendEntry alpha;
			};
			struct ColorTargetState
			{
				std::optional<BlendState> blend;
				TextureFormatEnum format = TextureFormatEnum::Undefined;
			};
			struct FragmentState
			{
				ShaderModule module;
				PointerRange<const ColorTargetState> targets;
			};
			std::optional<FragmentState> fragment;
		};

		struct CAGE_ENGINE_API SamplerDescriptor
		{
			StringView label;
			//Real lodMinClamp = 0;
			//Real lodMaxClamp = 32;
			uint32 maxAnisotropy = 1;
			AddressModeEnum addressModeU = AddressModeEnum::Undefined;
			AddressModeEnum addressModeV = AddressModeEnum::Undefined;
			AddressModeEnum addressModeW = AddressModeEnum::Undefined;
			FilterModeEnum magFilter = FilterModeEnum::Undefined;
			FilterModeEnum minFilter = FilterModeEnum::Undefined;
			FilterModeEnum mipmapFilter = FilterModeEnum::Undefined;
			//CompareFunctionEnum compare = CompareFunctionEnum::Undefined;
		};

		struct CAGE_ENGINE_API ShaderModuleDescriptor
		{
			StringView label;
			PointerRange<const uint32> spirvCode;
		};

		struct CAGE_ENGINE_API TexelCopyBufferLayout
		{
			//uint64 offset = 0;
			uint32 bytesPerRow = 0;
			uint32 rowsPerImage = 0;
		};

		struct CAGE_ENGINE_API TexelCopyBufferInfo
		{
			Buffer buffer;
			TexelCopyBufferLayout layout;
		};

		struct CAGE_ENGINE_API TexelCopyTextureInfo
		{
			Texture texture;
			//Vec3i origin;
			uint32 mipLevel = 0;
			TextureAspectEnum aspect = TextureAspectEnum::Undefined;
		};

		struct CAGE_ENGINE_API TextureDescriptor
		{
			StringView label;
			Vec3i size;
			uint32 mipLevelCount = 1;
			//uint32 sampleCount = 1;
			TextureUsageFlags usage = TextureUsageFlags::Undefined;
			TextureDimensionEnum dimension = TextureDimensionEnum::Undefined;
			TextureFormatEnum format = TextureFormatEnum::Undefined;
		};

		struct CAGE_ENGINE_API TextureViewDescriptor
		{
			StringView label;
			uint32 baseMipLevel = 0;
			uint32 mipLevelCount = m;
			uint32 baseArrayLayer = 0;
			uint32 arrayLayerCount = m;
			//TextureFormatEnum format = TextureFormatEnum::Undefined;
			TextureDimensionEnum dimension = TextureDimensionEnum::Undefined;
			//TextureAspectEnum aspect = TextureAspectEnum::Undefined;
			//TextureUsageFlags usage = TextureUsageFlags::Undefined;
		};

		struct CAGE_ENGINE_API VertexBufferLayout
		{
			struct VertexAttribute
			{
				uint64 offset = 0;
				uint32 shaderLocation = 0;
				VertexFormatEnum format = VertexFormatEnum::Undefined;
			};
			PointerRange<const VertexAttribute> attributes;

			uint32 arrayStride = 0;
			VertexStepModeEnum stepMode = VertexStepModeEnum::Undefined;
		};

		struct CAGE_ENGINE_API GpuDeviceDescriptor
		{
			StringView label;
			Window *window = nullptr;
		};

		///////////////////////////////////////////////////////////////////
		// implementation
		///////////////////////////////////////////////////////////////////

		template<class Callable>
		Future Buffer::mapAsync(MapModeEnum mapMode, uint64 offset, uint64 size, CallbackModeEnum callbackMode, Callable callable)
		{
			return {}; // todo
		}

		template<class Callable>
		Future Device::createRenderPipelineAsync(const RenderPipelineDescriptor &descriptor, CallbackModeEnum callbackMode, Callable callable)
		{
			// todo make actually async
			auto r = createRenderPipeline(descriptor);
			callable(StatusEnum::Success, std::move(r), "");
			return {}; // todo
		}

		template<class Callable>
		Future Device::onSubmittedWorkDone(CallbackModeEnum callbackMode, Callable callable)
		{
			return {}; // todo
		}

		CAGE_ENGINE_API Device newGpuDevice(const GpuDeviceDescriptor &desc);

		CAGE_ENGINE_API void logGpuMessage(SeverityEnum severity, StringView message);
	}
}

#endif
