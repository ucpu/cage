#ifndef guard_gpuInterface_sdrzuij4rt5e
#define guard_gpuInterface_sdrzuij4rt5e

#include <functional>
#include <optional>
#include <variant>

#include <svector.h>

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
		class PipelineLayoutImpl;
		class QuerySetImpl;
		class RenderPipelineImpl;
		class SamplerImpl;
		class ShaderModuleImpl;
		class TextureImpl;
		class TextureViewImpl;

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
			CAGE_FORCE_INLINE bool operator==(const GpuInterfaceHandle &other) const { return +ptr == +other.ptr; }

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
			PointerRange<char> getMappedRange() const;

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
			void pushDebugGroup(const AssetLabel &label);
			void popDebugGroup();
			CommandBuffer finishEncoding();

			// generic encoder

			//void clearBuffer(const Buffer &buffer, uint64 offset = 0, uint64 size = m);
			//void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void copyBufferToBuffer(const Buffer &source, uint64 sourceOffset, const Buffer &destination, uint64 destinationOffset, uint64 size);
			void copyBufferToTexture(const Buffer &source, uint64 sourceOffset, const TexelCopyTextureInfo &destination, Vec3i copySize);
			void copyTextureToBuffer(const TexelCopyTextureInfo &source, const Buffer &destination, uint64 destinationOffset, Vec3i copySize);
			//void copyTextureToTexture(const TexelCopyTextureInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize);
			void resolveQuerySet(const QuerySet &querySet, uint32 firstQuery, uint32 queryCount, const Buffer &destination, uint64 destinationOffset);
			void writeTimestamp(const QuerySet &querySet, uint32 queryIndex);

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
			QuerySet createQuerySet(const QuerySetDescriptor &descriptor);
			RenderPipeline createRenderPipeline(const RenderPipelineDescriptor &descriptor);
			Sampler createSampler(const SamplerDescriptor &descriptor);
			ShaderModule createShaderModule(const ShaderModuleDescriptor &descriptor);
			Texture createTexture(const TextureDescriptor &descriptor);

			template<class Callable>
			requires(std::is_invocable_r_v<void, Callable, StatusEnum, RenderPipeline>)
			void createRenderPipelineAsync(const RenderPipelineDescriptor &descriptor, Callable callable)
			{
				createRenderPipelineAsyncTypeErased(descriptor, { callable });
			}

			void writeBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, Vec3i extents);
			void writeTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, Vec3i extents);

			void setVsyncPreference(bool vsync, bool tripleBuffer);
			void submitAndPresent(PointerRange<const CommandBuffer> buffers, PointerRange<WindowPresentationDescriptor> windows);
			void submit(PointerRange<const CommandBuffer> buffers);
			void waitDeviceIdle();

			float getTimestampsConversion() const;
			MemoryStatus getMemoryStatus() const;

		private:
			void createRenderPipelineAsyncTypeErased(const RenderPipelineDescriptor &descriptor, std::function<void(StatusEnum, RenderPipeline)> callback);
		};

		class CAGE_ENGINE_API PipelineLayout : public GpuInterfaceHandle<PipelineLayout, PipelineLayoutImpl>
		{
		public:
		};

		class CAGE_ENGINE_API QuerySet : public GpuInterfaceHandle<QuerySet, QuerySetImpl>
		{
		public:
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

		class CAGE_ENGINE_API Texture : public GpuInterfaceHandle<Texture, TextureImpl>
		{
		public:
			TextureView createView(const TextureViewDescriptor &desc);

			Vec3i getResolution() const;
			uint32 getArrayLayersCount() const;
			uint32 getMipLevelsCount() const;
			//uint32 getSampleCount() const;
			TextureDimensionEnum getDimension() const;
			TextureFormatEnum getFormat() const;
			TextureUsageFlags getUsage() const;
		};

		class CAGE_ENGINE_API TextureView : public GpuInterfaceHandle<TextureView, TextureViewImpl>
		{
		public:
			Texture getTexture() const;
			uint32 getArrayLayersOffset() const;
			uint32 getArrayLayersCount() const;
			uint32 getMipLevelsOffset() const;
			uint32 getMipLevelsCount() const;
			TextureDimensionEnum getDimension() const;
		};

		///////////////////////////////////////////////////////////////////
		// descriptors
		///////////////////////////////////////////////////////////////////

		struct CAGE_ENGINE_API VertexBufferLayout
		{
			struct VertexAttribute
			{
				uint64 offset = 0;
				uint32 shaderLocation = 0;
				VertexFormatEnum format = VertexFormatEnum::Undefined;

				bool operator==(const VertexAttribute &) const = default;
			};
			ankerl::svector<VertexAttribute, 5> attributes;

			uint32 arrayStride = 0;

			bool operator==(const VertexBufferLayout &) const = default;
		};

		struct CAGE_ENGINE_API BindGroupDescriptor
		{
			AssetLabel label;

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
				TextureView view;
			};

			struct Entry
			{
				std::variant<std::monostate, BufferEntry, SamplerEntry, TextureEntry> data;
				uint32 binding = 0;
			};
			ankerl::svector<Entry, 10> entries;

			BindGroupLayout layout;
		};

		struct CAGE_ENGINE_API BindGroupLayoutDescriptor
		{
			AssetLabel label;

			struct BufferEntry
			{
				BufferBindingTypeEnum type = BufferBindingTypeEnum::Undefined;
				bool hasDynamicOffset = false;

				bool operator==(const BufferEntry &) const = default;
			};
			struct SamplerEntry
			{
				bool operator==(const SamplerEntry &) const = default;
			};
			struct TextureEntry
			{
				bool operator==(const TextureEntry &) const = default;
			};

			struct Entry
			{
				std::variant<std::monostate, BufferEntry, SamplerEntry, TextureEntry> data;
				uint32 binding = 0;
				ShaderStagesFlags shaderStages = ShaderStagesFlags::Undefined;

				bool operator==(const Entry &) const = default;
			};
			ankerl::svector<Entry, 10> entries;
		};

		struct CAGE_ENGINE_API BufferDescriptor
		{
			AssetLabel label;
			uint64 size = 0;
			BufferUsageFlags usage = BufferUsageFlags::Undefined;
		};

		struct CAGE_ENGINE_API CommandEncoderDescriptor
		{
			AssetLabel label;
		};

		struct CAGE_ENGINE_API PipelineLayoutDescriptor
		{
			AssetLabel label;
			ankerl::svector<BindGroupLayout, 3> bindGroupLayouts;
			//uint32 immediateSize = 0;
		};

		struct CAGE_ENGINE_API RenderPassDescriptor
		{
			AssetLabel label;

			struct ColorAttachment
			{
				TextureView view;
				//TextureView resolveTarget;
				Vec4 clearValue;
				LoadOpEnum loadOp = LoadOpEnum::Undefined;
				StoreOpEnum storeOp = StoreOpEnum::Undefined;
			};
			ankerl::svector<ColorAttachment, 1> colorAttachments;

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

			Vec2i targetResolution() const;
		};

		struct CAGE_ENGINE_API RenderPipelineDescriptor
		{
			AssetLabel label;
			PipelineLayout layout;

			struct VertexState
			{
				ShaderModule module;
				ankerl::svector<VertexBufferLayout, 1> buffers;
			};
			VertexState vertex;

			struct PrimitiveState
			{
				PrimitiveTopologyEnum topology = PrimitiveTopologyEnum::Undefined;
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
				ankerl::svector<ColorTargetState, 1> targets;
			};
			std::optional<FragmentState> fragment;
		};

		struct CAGE_ENGINE_API SamplerDescriptor
		{
			AssetLabel label;
			uint32 maxAnisotropy = 1;
			AddressModeEnum addressModeU = AddressModeEnum::Undefined;
			AddressModeEnum addressModeV = AddressModeEnum::Undefined;
			AddressModeEnum addressModeW = AddressModeEnum::Undefined;
			FilterModeEnum magFilter = FilterModeEnum::Undefined;
			FilterModeEnum minFilter = FilterModeEnum::Undefined;
			FilterModeEnum mipmapFilter = FilterModeEnum::Undefined;
			CompareFunctionEnum compare = CompareFunctionEnum::Undefined;
		};

		struct CAGE_ENGINE_API ShaderModuleDescriptor
		{
			AssetLabel label;
			PointerRange<const uint32> spirvCode;
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
			AssetLabel label;
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
			AssetLabel label;
			uint32 arrayLayersOffset = 0;
			uint32 arrayLayersCount = 1;
			uint32 mipLevelsOffset = 0;
			uint32 mipLevelsCount = 1;
			TextureDimensionEnum dimension = TextureDimensionEnum::Undefined;
		};

		struct CAGE_ENGINE_API QuerySetDescriptor
		{
			AssetLabel label;
			uint32 count = 0;
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
			AssetLabel label;
			Window *window = nullptr;
		};

		struct CAGE_ENGINE_API MemoryStatus
		{
			uint64 deviceUsage = 0;
			uint64 deviceBudget = 0;
			uint64 totalUsage = 0;
			uint64 totalBudget = 0;
		};

		///////////////////////////////////////////////////////////////////
		// implementation
		///////////////////////////////////////////////////////////////////

		CAGE_ENGINE_API Device newGpuDevice(const GpuDeviceDescriptor &desc);
	}
}

#endif
