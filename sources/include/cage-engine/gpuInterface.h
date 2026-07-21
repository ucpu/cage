#ifndef guard_gpuInterface_sdrzuij4rt5e
#define guard_gpuInterface_sdrzuij4rt5e

#include <functional>
#include <optional>
#include <variant>

#include <cage-engine/gpuCore.h>

namespace cage
{
	class Window;

	namespace gpu
	{
		///////////////////////////////////////////////////////////////////
		// forward declare implementation
		///////////////////////////////////////////////////////////////////

		class BindGroupImpl;
		class BindGroupLayoutImpl;
		class BufferImpl;
		class CommandBufferImpl;
		class CommandEncoderImpl;
		class DeviceImpl;
		class RenderPipelineImpl;
		class SamplerImpl;
		class ShaderModuleImpl;
		class PipelineLayoutImpl;
		class TextureImpl;
		class TextureViewImpl;

		///////////////////////////////////////////////////////////////////
		// StringView
		///////////////////////////////////////////////////////////////////

		struct CAGE_ENGINE_API StringView
		{
			PointerRange<const char> str;

			StringView() {}
			StringView(const char *ptr);
			StringView(const String &ptr) : str(ptr) {}
			StringView(const AssetLabel &ptr) : str(ptr) {}

			StringView(const StringView &) noexcept = default;
			StringView &operator=(const StringView &) noexcept = default;
		};

		///////////////////////////////////////////////////////////////////
		// GpuInterfaceHandle
		///////////////////////////////////////////////////////////////////

		template<class Crtp, class Impl>
		class GpuInterfaceHandle
		{
		public:
			CAGE_FORCE_INLINE GpuInterfaceHandle() noexcept {}
			CAGE_FORCE_INLINE GpuInterfaceHandle(Holder<Impl> &&impl) : ptr(std::move(impl)) {}
			CAGE_FORCE_INLINE GpuInterfaceHandle(const GpuInterfaceHandle &other) : ptr(other.ptr.share()) {}
			CAGE_FORCE_INLINE GpuInterfaceHandle(GpuInterfaceHandle &&other) noexcept : ptr(std::move(other.ptr)) {}
			CAGE_FORCE_INLINE GpuInterfaceHandle &operator=(const GpuInterfaceHandle &other)
			{
				ptr = other.ptr.share();
				return *this;
			}
			CAGE_FORCE_INLINE GpuInterfaceHandle &operator=(GpuInterfaceHandle &&other) noexcept
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

			CAGE_FORCE_INLINE Holder<void> getVoidHolder() const { return ptr.share().template cast<void>(); }

		private:
			Holder<Impl> ptr;
			friend Crtp;
		};

		///////////////////////////////////////////////////////////////////
		// gpu resources
		///////////////////////////////////////////////////////////////////

		class CAGE_ENGINE_API BindGroup : public GpuInterfaceHandle<BindGroup, BindGroupImpl>
		{
		public:
		};

		class CAGE_ENGINE_API BindGroupLayout : public GpuInterfaceHandle<BindGroupLayout, BindGroupLayoutImpl>
		{
		public:
		};

		class CAGE_ENGINE_API Buffer : public GpuInterfaceHandle<Buffer, BufferImpl>
		{
		public:
			uint64 getSize() const;
			BufferUsageFlags getUsage() const;
			PointerRange<char> getMappedRange();

			void flush(); // makes cpu writes visible to gpu
			void invalidate(); // makes gpu data visible to cpu for reading
		};

		class CAGE_ENGINE_API CommandBuffer : public GpuInterfaceHandle<CommandBuffer, CommandBufferImpl>
		{
		public:
		};

		class CAGE_ENGINE_API CommandEncoder : public GpuInterfaceHandle<CommandEncoder, CommandEncoderImpl>
		{
		public:
			EncoderModeEnum mode() const;
			void pushDebugGroup(StringView label);
			void popDebugGroup();
			CommandBuffer finishEncoding();

			// generic encoder

			//void clearBuffer(const Buffer &buffer, uint64 offset = 0, uint64 size = m);
			//void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void copyBufferToBuffer(const Buffer &source, uint64 sourceOffset, const Buffer &destination, uint64 destinationOffset, uint64 size);
			void copyBufferToTexture(const TexelCopyBufferInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize);
			void copyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize);
			//void copyTextureToTexture(const TexelCopyTextureInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize);
			//void resolveQuerySet(const QuerySet &querySet, uint32 firstQuery, uint32 queryCount, const Buffer &destination, uint64 destinationOffset);
			//void writeTimestamp(const QuerySet &querySet, uint32 queryIndex);

			// render pass encoder

			void beginRenderPass(const RenderPassDescriptor &descriptor);
			void endRenderPass();

			void draw(uint32 verticesCount, uint32 instancesCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0);
			void drawIndexed(uint32 indicesCount, uint32 instancesCount = 1, uint32 firstIndex = 0, sint32 baseVertex = 0, uint32 firstInstance = 0);
			//void drawIndexedIndirect(const Buffer &indirectBuffer, uint64 indirectOffset);
			//void drawIndirect(const Buffer &indirectBuffer, uint64 indirectOffset);
			//void multiDrawIndexedIndirect(const Buffer &indirectBuffer, uint64 indirectOffset, uint32 maxDrawCount, const Buffer &drawCountBuffer = {}, uint64 drawCountBufferOffset = 0);
			//void multiDrawIndirect(const Buffer &indirectBuffer, uint64 indirectOffset, uint32 maxDrawCount, const Buffer &drawCountBuffer = {}, uint64 drawCountBufferOffset = 0);
			//void pixelLocalStorageBarrier();
			void setBindGroup(uint32 binding, const BindGroup &group = {});
			void setBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets);
			//void setBlendConstant(Vec4 color);
			//void setImmediates(uint32 offset, PointerRange<const char> data);
			void setIndexBuffer(const Buffer &buffer, IndexFormatEnum format, uint64 offset = 0, uint64 size = m);
			void setPipeline(const RenderPipeline &pipeline);
			void setScissorRect(uint32 x, uint32 y, uint32 w, uint32 h);
			//void setStencilReference(uint32 reference);
			void setVertexBuffer(uint32 slot, const Buffer &buffer = {}, uint64 offset = 0, uint64 size = m);
			void setViewport(Real x, Real y, Real width, Real height, Real minDepth = 0, Real maxDepth = 1);
			//void writeTimestamp(const QuerySet &querySet, uint32 queryIndex);

			// compute pass encoder
		};

		class CAGE_ENGINE_API Device : public GpuInterfaceHandle<Device, DeviceImpl>
		{
		public:
			BindGroup createBindGroup(const BindGroupDescriptor &descriptor);
			BindGroupLayout createBindGroupLayout(const BindGroupLayoutDescriptor &descriptor);
			Buffer createBuffer(const BufferDescriptor &descriptor);
			CommandEncoder createCommandEncoder(const CommandEncoderDescriptor &descriptor);
			PipelineLayout createPipelineLayout(const PipelineLayoutDescriptor &descriptor);
			//QuerySet createQuerySet(const QuerySetDescriptor &descriptor);
			RenderPipeline createRenderPipeline(const RenderPipelineDescriptor &descriptor);
			Sampler createSampler(const SamplerDescriptor &descriptor);
			ShaderModule createShaderModule(const ShaderModuleDescriptor &descriptor);
			Texture createTexture(const TextureDescriptor &descriptor);

			template<class Callable>
			requires(std::is_invocable_r_v<void, Callable, StatusEnum, RenderPipeline, StringView>)
			void createRenderPipelineAsync(const RenderPipelineDescriptor &descriptor, Callable callable)
			{
				createRenderPipelineAsyncTypeErased(descriptor, { callable });
			}

			void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents);

			void submitAndPresentWindows(PointerRange<const CommandBuffer> buffers, PointerRange<WindowPresentationDescriptor> windows);

		private:
			void createRenderPipelineAsyncTypeErased(const RenderPipelineDescriptor &descriptor, std::function<void(StatusEnum, RenderPipeline, StringView)> callback);
		};

		class CAGE_ENGINE_API RenderPipeline : public GpuInterfaceHandle<RenderPipeline, RenderPipelineImpl>
		{
		public:
		};

		class CAGE_ENGINE_API Sampler : public GpuInterfaceHandle<Sampler, SamplerImpl>
		{
		public:
		};

		class CAGE_ENGINE_API ShaderModule : public GpuInterfaceHandle<ShaderModule, ShaderModuleImpl>
		{
		public:
		};

		class CAGE_ENGINE_API PipelineLayout : public GpuInterfaceHandle<PipelineLayout, PipelineLayoutImpl>
		{
		public:
		};

		class CAGE_ENGINE_API Texture : public GpuInterfaceHandle<Texture, TextureImpl>
		{
		public:
			TextureView createView(const TextureViewDescriptor &desc);

			Vec3i getResolution() const;
			uint32 getArrayLayers() const;
			uint32 getMipLevels() const;
			//uint32 getSampleCount() const;
			TextureDimensionEnum getDimension() const;
			TextureFormatEnum getFormat() const;
			TextureUsageFlags getUsage() const;

			//void pin(TextureUsageFlags usage);
			//void unpin();
		};

		class CAGE_ENGINE_API TextureView : public GpuInterfaceHandle<TextureView, TextureViewImpl>
		{
		public:
			Texture getTexture() const;
			uint32 getBaseArrayLayer() const;
			uint32 getArrayLayers() const;
			uint32 getBaseMipLevel() const;
			uint32 getMipLevels() const;
			TextureDimensionEnum getDimension() const;
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
		};

		struct CAGE_ENGINE_API CommandEncoderDescriptor
		{
			StringView label;
		};

		struct CAGE_ENGINE_API PipelineLayoutDescriptor
		{
			StringView label;
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
			StringView label;
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
				FrontFaceEnum frontFace = FrontFaceEnum::CCW;
				CullModeEnum cullMode = CullModeEnum::Undefined;
			};
			PrimitiveState primitive;

			//struct MultisampleState
			//{
			//	uint32 count = 1;
			//	uint32 mask = 0xFFFFFFFF;
			//	bool alphaToCoverageEnabled = false;
			//};
			//std::optional<MultisampleState> multisample;

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
			uint64 offset = 0;
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
			Vec3i origin;
			uint32 arrayLayersOffset = 0;
			uint32 arrayLayersCount = 1;
			uint32 mipLevel = 0;
		};

		struct CAGE_ENGINE_API TextureDescriptor
		{
			StringView label;
			Vec3i resolution = Vec3i(0, 0, 1);
			uint32 arrayLayersCount = 1;
			uint32 mipLevelsCount = 1;
			//uint32 samplesCount = 1;
			TextureDimensionEnum dimension = TextureDimensionEnum::Undefined;
			TextureFormatEnum format = TextureFormatEnum::Undefined;
			TextureUsageFlags usage = TextureUsageFlags::Undefined;
		};

		struct CAGE_ENGINE_API TextureViewDescriptor
		{
			StringView label;
			uint32 arrayLayersOffset = 0;
			uint32 arrayLayersCount = 1;
			uint32 mipLevelsOffset = 0;
			uint32 mipLevelsCount = 1;
			TextureDimensionEnum dimension = TextureDimensionEnum::Undefined;
			//TextureFormatEnum format = TextureFormatEnum::Undefined;
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
		};

		struct CAGE_ENGINE_API WindowPresentationDescriptor
		{
			// in
			Window *window = nullptr;
			// out
			Texture texture;
		};

		struct CAGE_ENGINE_API GpuDeviceDescriptor
		{
			StringView label;
			Window *window = nullptr;
		};

		///////////////////////////////////////////////////////////////////
		// implementation
		///////////////////////////////////////////////////////////////////

		CAGE_ENGINE_API Device newGpuDevice(const GpuDeviceDescriptor &desc);

		CAGE_ENGINE_API void logGpuMessage(SeverityEnum severity, StringView message);
	}
}

#endif
