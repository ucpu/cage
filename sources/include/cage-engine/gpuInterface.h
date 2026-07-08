#ifndef guard_gpuInterface_sdrzuij4rt5e
#define guard_gpuInterface_sdrzuij4rt5e

#include <optional>

#include <cage-engine/gpuCore.h>

namespace cage
{
	namespace gpu
	{
		///////////////////////////////////////////////////////////////////
		// basic primitives
		///////////////////////////////////////////////////////////////////

		struct CAGE_ENGINE_API StringView
		{
			PointerRange<const char> str;

			StringView();
			StringView(const char *ptr);
		};

		struct CAGE_ENGINE_API Future
		{};

		///////////////////////////////////////////////////////////////////
		// gpu resource handles
		///////////////////////////////////////////////////////////////////

		template<class CRTP>
		class GpuResource
		{
		public:
			operator bool() const { return !!ptr; }
			void *Get() const { return ptr; }

		private:
			void *ptr = nullptr;
		};

		class CAGE_ENGINE_API BindGroup : public GpuResource<BindGroup>
		{
		public:
		};

		class CAGE_ENGINE_API BindGroupLayout : public GpuResource<BindGroupLayout>
		{
		public:
		};

		class CAGE_ENGINE_API Buffer : public GpuResource<Buffer>
		{
		public:
			uint64 GetSize() const;
			BufferUsage GetUsage() const;
			//BufferMapState GetMapState() const;
			PointerRange<char> GetMappedRange();
			template<class Callable>
			Future MapAsync(MapMode mapMode, uint64 offset, uint64 size, CallbackMode callbackMode, Callable callable);
			void Unmap();
		};

		class CAGE_ENGINE_API CommandBuffer : public GpuResource<CommandBuffer>
		{
		public:
		};

		class CAGE_ENGINE_API CommandEncoder : public GpuResource<CommandEncoder>
		{
		public:
			RenderPassEncoder BeginRenderPass(const RenderPassDescriptor &descriptor);

			//void ClearBuffer(const Buffer &buffer, uint64 offset = 0, uint64 size = m);
			//void WriteBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);

			//void CopyBufferToBuffer(const Buffer &source, uint64 sourceOffset, const Buffer &destination, uint64 destinationOffset, uint64 size);
			//void CopyBufferToTexture(const TexelCopyBufferInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize);
			void CopyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, Vec3i copySize);
			//void CopyTextureToTexture(const TexelCopyTextureInfo &source, const TexelCopyTextureInfo &destination, Vec3i copySize);

			//void ResolveQuerySet(const QuerySet &querySet, uint32 firstQuery, uint32 queryCount, const Buffer &destination, uint64 destinationOffset);
			//void WriteTimestamp(const QuerySet &querySet, uint32 queryIndex);

			CommandBuffer Finish();
		};

		class CAGE_ENGINE_API Device : public GpuResource<Device>
		{
		public:
			BindGroup CreateBindGroup(const BindGroupDescriptor &descriptor);
			BindGroupLayout CreateBindGroupLayout(const BindGroupLayoutDescriptor &descriptor);
			Buffer CreateBuffer(const BufferDescriptor &descriptor);
			CommandEncoder CreateCommandEncoder(const CommandEncoderDescriptor &descriptor);
			PipelineLayout CreatePipelineLayout(const PipelineLayoutDescriptor &descriptor);
			//QuerySet CreateQuerySet(const QuerySetDescriptor &descriptor);
			//RenderPipeline CreateRenderPipeline(const RenderPipelineDescriptor &descriptor);
			template<class Callable>
			RenderPipeline CreateRenderPipelineAsync(const RenderPipelineDescriptor &descriptor, CallbackMode callbackMode, Callable callable);
			Sampler CreateSampler(const SamplerDescriptor &descriptor);
			ShaderModule CreateShaderModule(const ShaderModuleDescriptor &descriptor);
			Texture CreateTexture(const TextureDescriptor &descriptor);
			//Queue GetQueue();
			//void Tick();
		};

		class CAGE_ENGINE_API Queue : public GpuResource<Queue>
		{
		public:
			template<class Callable>
			Future OnSubmittedWorkDone(CallbackMode callbackMode, Callable callable);
			//void Submit(PointerRange<const CommandBuffer> commands);
			void WriteBuffer(const Buffer &buffer, uint64 offset, PointerRange<const char> data);
			void WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, Vec3i extents);
			void WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, Vec3i extents);
		};

		class CAGE_ENGINE_API RenderPassEncoder : public GpuResource<RenderPassEncoder>
		{
		public:
			void Draw(uint32 verticesCount, uint32 instancesCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0);
			void DrawIndexed(uint32 indicesCount, uint32 instancesCount = 1, uint32 firstIndex = 0, sint32 baseVertex = 0, uint32 firstInstance = 0);
			//void DrawIndexedIndirect(const Buffer &indirectBuffer, uint64 indirectOffset);
			//void DrawIndirect(const Buffer &indirectBuffer, uint64 indirectOffset);
			void End();
			//void MultiDrawIndexedIndirect(const Buffer &indirectBuffer, uint64 indirectOffset, uint32 maxDrawCount, const Buffer &drawCountBuffer = {}, uint64 drawCountBufferOffset = 0);
			//void MultiDrawIndirect(const Buffer &indirectBuffer, uint64 indirectOffset, uint32 maxDrawCount, const Buffer &drawCountBuffer = {}, uint64 drawCountBufferOffset = 0);
			//void PixelLocalStorageBarrier();
			void PopDebugGroup();
			void PushDebugGroup(StringView label);
			void SetBindGroup(uint32 binding, const BindGroup &group = {});
			void SetBindGroup(uint32 binding, const BindGroup &group, PointerRange<const uint32> dynamicOffsets);
			//void SetBlendConstant(Vec4 color);
			//void SetImmediates(uint32 offset, PointerRange<const char> data);
			void SetIndexBuffer(const Buffer &buffer, IndexFormat format, uint64 offset = 0, uint64 size = m);
			void SetPipeline(const RenderPipeline &pipeline);
			void SetScissorRect(uint32 x, uint32 y, uint32 w, uint32 h);
			//void SetStencilReference(uint32 reference);
			void SetVertexBuffer(uint32 slot, const Buffer &buffer = {}, uint64 offset = 0, uint64 size = m);
			//void SetViewport(Real x, Real y, Real width, Real height, Real minDepth, Real maxDepth);
			//void WriteTimestamp(const QuerySet &querySet, uint32 queryIndex);
		};

		class CAGE_ENGINE_API RenderPipeline : public GpuResource<RenderPipeline>
		{
		public:
			//BindGroupLayout GetBindGroupLayout(uint32 groupIndex);
		};

		class CAGE_ENGINE_API Sampler : public GpuResource<Sampler>
		{
		public:
		};

		class CAGE_ENGINE_API ShaderModule : public GpuResource<ShaderModule>
		{
		public:
		};

		class CAGE_ENGINE_API PipelineLayout : public GpuResource<PipelineLayout>
		{
		public:
		};

		class CAGE_ENGINE_API Texture : public GpuResource<Texture>
		{
		public:
			TextureView CreateView(const TextureViewDescriptor &desc);
			uint32 GetDepthOrArrayLayers() const;
			TextureDimension GetDimension() const;
			TextureFormat GetFormat() const;
			uint32 GetHeight() const;
			uint32 GetMipLevelCount() const;
			//uint32 GetSampleCount() const;
			//TextureViewDimension GetTextureBindingViewDimension() const;
			//TextureUsage GetUsage() const;
			uint32 GetWidth() const;
			//void Pin(TextureUsage usage);
			//void Unpin();
		};

		class CAGE_ENGINE_API TextureView : public GpuResource<TextureView>
		{
		public:
		};

		///////////////////////////////////////////////////////////////////
		// descriptors
		///////////////////////////////////////////////////////////////////

		struct CAGE_ENGINE_API BindGroupDescriptor
		{
			StringView label;

			struct Entry
			{
				Buffer buffer;
				TextureView textureView;
				Sampler sampler;
				uint64 offset = 0;
				uint64 size = 0; // use -1 to use the whole buffer
				uint32 binding = 0;
			};
			PointerRange<const Entry> entries;

			BindGroupLayout layout;
		};

		struct CAGE_ENGINE_API BindGroupLayoutDescriptor
		{
			StringView label;

			struct Entry
			{
				struct BufferEntry
				{
					BufferBindingType type = BufferBindingType::Undefined;
					bool hasDynamicOffset = false;
				};
				BufferEntry buffer;

				struct SamplerEntry
				{
					SamplerBindingType type = SamplerBindingType::Undefined;
				};
				SamplerEntry sampler;

				struct TextureEntry
				{
					TextureSampleType sampleType = TextureSampleType::Undefined;
					TextureViewDimension viewDimension = TextureViewDimension::Undefined;
					//bool multisampled = false;
				};
				TextureEntry texture;

				uint32 binding = 0;
				ShaderStage visibility = ShaderStage::Undefined;
			};
			PointerRange<const Entry> entries;
		};

		struct CAGE_ENGINE_API BufferDescriptor
		{
			StringView label;
			uint64 size = 0;
			BufferUsage usage = BufferUsage::Undefined;
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
				LoadOp loadOp = LoadOp::Undefined;
				StoreOp storeOp = StoreOp::Undefined;
			};
			PointerRange<const ColorAttachment> colorAttachments;

			struct DepthStencilAttachment
			{
				TextureView view;
				Real depthClearValue;
				//uint32 stencilClearValue = 0;
				LoadOp depthLoadOp = LoadOp::Undefined;
				StoreOp depthStoreOp = StoreOp::Undefined;
				//LoadOp stencilLoadOp = LoadOp::Undefined;
				//StoreOp stencilStoreOp = StoreOp::Undefined;
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
				PrimitiveTopology topology = PrimitiveTopology::Undefined;
				//IndexFormat stripIndexFormat = IndexFormat::Undefined;
				//FrontFace frontFace = FrontFace::Undefined;
				CullMode cullMode = CullMode::Undefined;
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
				TextureFormat format = TextureFormat::Undefined;
				CompareFunction depthCompare = CompareFunction::Undefined;
				std::optional<bool> depthWriteEnabled;
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
					BlendOperation operation = BlendOperation::Undefined;
					BlendFactor srcFactor = BlendFactor::Undefined;
					BlendFactor dstFactor = BlendFactor::Undefined;
				};
				BlendEntry color;
				BlendEntry alpha;
			};
			struct ColorTargetState
			{
				std::optional<BlendState> blend;
				TextureFormat format = TextureFormat::Undefined;
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
			AddressMode addressModeU = AddressMode::Undefined;
			AddressMode addressModeV = AddressMode::Undefined;
			AddressMode addressModeW = AddressMode::Undefined;
			FilterMode magFilter = FilterMode::Undefined;
			FilterMode minFilter = FilterMode::Undefined;
			MipmapFilterMode mipmapFilter = MipmapFilterMode::Undefined;
			//CompareFunction compare = CompareFunction::Undefined;
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
			TextureAspect aspect = TextureAspect::Undefined;
		};

		struct CAGE_ENGINE_API TextureDescriptor
		{
			StringView label;
			Vec3i size;
			uint32 mipLevelCount = 1;
			//uint32 sampleCount = 1;
			TextureUsage usage = TextureUsage::Undefined;
			TextureDimension dimension = TextureDimension::Undefined;
			TextureFormat format = TextureFormat::Undefined;
		};

		struct CAGE_ENGINE_API TextureViewDescriptor
		{
			StringView label;
			uint32 baseMipLevel = 0;
			uint32 mipLevelCount = m;
			uint32 baseArrayLayer = 0;
			uint32 arrayLayerCount = m;
			//TextureFormat format = TextureFormat::Undefined;
			TextureViewDimension dimension = TextureViewDimension::Undefined;
			//TextureAspect aspect = TextureAspect::Undefined;
			//TextureUsage usage = TextureUsage::Undefined;
		};

		struct CAGE_ENGINE_API VertexBufferLayout
		{
			struct VertexAttribute
			{
				uint64 offset = 0;
				uint32 shaderLocation = 0;
				VertexFormat format = VertexFormat::Undefined;
			};
			PointerRange<const VertexAttribute> attributes;

			uint32 arrayStride = 0;
			VertexStepMode stepMode = VertexStepMode::Undefined;
		};

		///////////////////////////////////////////////////////////////////
		// implementation
		///////////////////////////////////////////////////////////////////

		template<class Callable>
		Future Buffer::MapAsync(MapMode mapMode, uint64 offset, uint64 size, CallbackMode callbackMode, Callable callable)
		{
			return {}; // todo
		}

		template<class Callable>
		RenderPipeline Device::CreateRenderPipelineAsync(const RenderPipelineDescriptor &descriptor, CallbackMode callbackMode, Callable callable)
		{
			return {}; // todo
		}

		template<class Callable>
		Future Queue::OnSubmittedWorkDone(CallbackMode callbackMode, Callable callable)
		{
			return {}; // todo
		}
	}
}

#endif
