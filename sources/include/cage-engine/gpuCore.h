#ifndef guard_gpuCore_erzike5
#define guard_gpuCore_erzike5

#include <cage-engine/core.h>

namespace cage
{
	namespace gpu
	{
		// enums

		enum class BufferUsage
		{
			Undefined = 0,
			MapRead,
			CopyDst,
			Vertex,
			Index,
			Uniform,
			Storage,
		};

		enum class TextureFormat
		{
			Undefined = 0,
			BC7RGBAUnormSrgb,
			BC1RGBAUnorm,
			BC1RGBAUnormSrgb,
			BC4RSnorm,
			BC2RGBAUnorm,
			BC2RGBAUnormSrgb,
			BC3RGBAUnorm,
			BC3RGBAUnormSrgb,
			BC5RGSnorm,
			BC6HRGBUfloat,
			BC6HRGBFloat,
			BC4RUnorm,
			BC5RGUnorm,
			BC7RGBAUnorm,
			RGBA8UnormSrgb,
			R8Unorm,
			RG8Unorm,
			RGBA8Unorm,
			R16Unorm,
			RG16Unorm,
			RGBA16Unorm,
			R32Float,
			RG32Float,
			RGBA32Float,
			Depth16Unorm,
			Depth24Plus,
			Depth24PlusStencil8,
			Depth32Float,
			Depth32FloatStencil8,
			R8Sint,
			R8Uint,
			RG8Sint,
			RG8Uint,
			RGBA8Sint,
			RGBA8Uint,
			R16Sint,
			R16Uint,
			RG16Sint,
			RG16Uint,
			RGBA16Sint,
			RGBA16Uint,
			R32Sint,
			R32Uint,
			RG32Sint,
			RG32Uint,
			RGBA32Sint,
			RGBA32Uint,
			RGBA16Float,
			R16Float,
		};

		enum class TextureUsage
		{
			Undefined = 0,
			CopyDst,
			TextureBinding,
			RenderAttachment,
		};

		enum class TextureDimension
		{
			Undefined = 0,
			e2D,
			e2DArray,
			Cube,
			CubeArray,
			e3D,
		};
		using TextureViewDimension = TextureDimension;

		enum class TextureAspect
		{
			Undefined = 0,
			All,
		};

		enum class TextureSampleType
		{
			Undefined = 0,
			Float,
			UnfilterableFloat,
		};

		enum class SamplerBindingType
		{
			Undefined = 0,
			Filtering,
			NonFiltering,
		};

		enum class MipmapFilterMode
		{
			Undefined = 0,
			Linear,
			Nearest,
		};

		enum class PrimitiveTopology
		{
			Undefined = 0,
			PointList,
			LineList,
			TriangleList,
		};

		enum class FilterMode
		{
			Undefined = 0,
			Nearest = 1,
			Linear = 2,
		};

		enum class AddressMode
		{
			Undefined = 0,
			ClampToEdge = 1,
			Repeat = 2,
			MirrorRepeat = 3,
		};

		enum class ShaderStage
		{
			Undefined = 0,
			Vertex,
			Fragment,
		};

		enum class BufferBindingType
		{
			Undefined = 0,
			Uniform,
			ReadOnlyStorage,
		};

		enum class CallbackMode
		{
			Undefined = 0,
			AllowProcessEvents,
			WaitAnyOnly,
		};

		enum class QueueWorkDoneStatus
		{
			Undefined = 0,
			Success,
		};

		enum class LoadOp
		{
			Undefined = 0,
			Clear,
			Load,
		};

		enum class StoreOp
		{
			Undefined = 0,
			Store,
		};

		enum class IndexFormat
		{
			Undefined = 0,
			Uint32,
		};

		enum class VertexFormat
		{
			Undefined = 0,
			Float32x3,
			Uint32x4,
			Float32x4,
			Float32x2,
		};

		enum class VertexStepMode
		{
			Undefined = 0,
			Vertex,
		};

		enum class BlendOperation
		{
			Undefined = 0,
			Add,
		};

		enum class BlendFactor
		{
			Undefined = 0,
			One,
			Zero,
			SrcAlpha,
			OneMinusSrcAlpha,
		};

		enum class CompareFunction
		{
			Undefined = 0,
			Never,
			Less, // default
			Equal,
			LessEqual,
			Greater,
			NotEqual,
			GreaterEqual,
			Always,
		};

		enum class CullMode
		{
			Undefined = 0,
			Back,
			None,
		};

		enum class CreatePipelineAsyncStatus
		{
			Undefined = 0,
			Success,
		};

		enum class MapMode
		{
			Undefined = 0,
			Read,
		};

		enum class MapAsyncStatus
		{
			Undefined = 0,
			Success,
		};

		// forward declarations

		class Buffer;
		class Texture;
		class TextureView;
		class Sampler;
		class CommandBuffer;
		class RenderPipelineLayout;
		class RenderPipeline;
		class CommandEncoder;
		class RenderPassEncoder;
		class BindGroupLayout;
		class BindGroup;
		class Device;
		class Queue;
		class ShaderModule;

		struct Extent3D;
		struct VertexBufferLayout;
		struct BufferDescriptor;
		struct TexelCopyTextureInfo;
		struct TexelCopyBufferInfo;
		struct TexelCopyBufferLayout;
		struct TextureDescriptor;
		struct TextureViewDescriptor;
		struct SamplerDescriptor;
		struct BindGroupLayoutEntry;
		struct BindGroupLayoutDescriptor;
		struct BindGroupEntry;
		struct BindGroupDescriptor;
		struct CommandEncoderDescriptor;
		struct RenderPassColorAttachment;
		struct RenderPassDepthStencilAttachment;
		struct RenderPassDescriptor;
		struct ShaderModuleDescriptor;
		struct VertexAttribute;
		struct BlendState;
		struct DepthStencilState;
		struct ColorTargetState;
		struct FragmentState;
		struct RenderPipelineDescriptor;
		struct PipelineLayoutDescriptor;

		// basic primitives

		struct CAGE_ENGINE_API StringView
		{
			PointerRange<const char> str;

			StringView();
			StringView(const char *ptr);
		};

		struct CAGE_ENGINE_API Future
		{};

		// gpu resource handles

		template<class CRTP>
		class GpuResource
		{
		public:
			operator bool() const { return !!ptr; }
			void *Get() const { return ptr; }

		private:
			void *ptr = nullptr;
		};

		class CAGE_ENGINE_API Buffer : public GpuResource<Buffer>
		{
		public:
			uintPtr GetSize() const;
			void *GetConstMappedRange();
			void Unmap();

			template<class Callable>
			Future MapAsync(MapMode mapMode, uintPtr offset, uintPtr size, CallbackMode callbackMode, Callable callable)
			{
				return {}; // todo
			}
		};

		class CAGE_ENGINE_API Texture : public GpuResource<Texture>
		{
		public:
			TextureFormat GetFormat() const;
			TextureView CreateView(const TextureViewDescriptor &desc);
			uint32 GetWidth() const;
			uint32 GetHeight() const;
			uint32 GetDepthOrArrayLayers() const;
			uint32 GetMipLevelCount() const;
			TextureDimension GetDimension() const;
		};

		class CAGE_ENGINE_API TextureView : public GpuResource<TextureView>
		{
		public:
		};

		class CAGE_ENGINE_API Sampler : public GpuResource<Sampler>
		{
		public:
		};

		class CAGE_ENGINE_API BindGroupLayout : public GpuResource<BindGroupLayout>
		{
		public:
		};

		class CAGE_ENGINE_API BindGroup : public GpuResource<BindGroup>
		{
		public:
		};

		class CAGE_ENGINE_API CommandBuffer : public GpuResource<CommandBuffer>
		{
		public:
		};

		class CAGE_ENGINE_API RenderPipelineLayout : public GpuResource<RenderPipelineLayout>
		{
		public:
		};

		class CAGE_ENGINE_API RenderPipeline : public GpuResource<RenderPipeline>
		{
		public:
		};

		class CAGE_ENGINE_API CommandEncoder : public GpuResource<CommandEncoder>
		{
		public:
			RenderPassEncoder BeginRenderPass(const RenderPassDescriptor &desc);
			CommandBuffer Finish();
			void CopyTextureToBuffer(const TexelCopyTextureInfo &source, const TexelCopyBufferInfo &destination, const Extent3D &copySize);
		};

		class CAGE_ENGINE_API RenderPassEncoder : public GpuResource<RenderPassEncoder>
		{
		public:
			void PushDebugGroup(StringView label);
			void PopDebugGroup();
			void End();
			void SetScissorRect(Real x, Real y, Real w, Real h);
			void SetPipeline(RenderPipeline pipeline);
			void SetBindGroup(uint32 binding, BindGroup group);
			void SetBindGroup(uint32 binding, BindGroup group, uint32 dynamicBuffersCount, PointerRange<const uint32> dynamicOffsets);
			void SetVertexBuffer(uint32 index, Buffer buffer);
			void SetIndexBuffer(Buffer buffer, IndexFormat format, uint32 indicesOffset);
			void DrawIndexed(uint32 indicesCount, uint32 instancesCount);
			void Draw(uint32 verticesCount, uint32 instancesCount);
		};

		class CAGE_ENGINE_API Device : public GpuResource<Device>
		{
		public:
			Buffer CreateBuffer(const BufferDescriptor &desc);
			Texture CreateTexture(const TextureDescriptor &desc);
			Sampler CreateSampler(const SamplerDescriptor &desc);
			BindGroupLayout CreateBindGroupLayout(const BindGroupLayoutDescriptor &desc);
			BindGroup CreateBindGroup(const BindGroupDescriptor &desc);
			CommandEncoder CreateCommandEncoder(const CommandEncoderDescriptor &desc);
			ShaderModule CreateShaderModule(const ShaderModuleDescriptor &desc);
			RenderPipelineLayout CreatePipelineLayout(const PipelineLayoutDescriptor &desc);

			template<class Callable>
			RenderPipeline CreateRenderPipelineAsync(const RenderPipelineDescriptor &desc, CallbackMode mode, Callable callable)
			{
				return {}; // todo
			}
		};

		class CAGE_ENGINE_API Queue : public GpuResource<Queue>
		{
		public:
			void WriteBuffer(Buffer buffer, uintPtr offset, PointerRange<const char> data);
			void WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const char> data, const TexelCopyBufferLayout &layout, const Extent3D &extents);
			void WriteTexture(const TexelCopyTextureInfo &dest, PointerRange<const uint8> data, const TexelCopyBufferLayout &layout, const Extent3D &extents);

			template<class Callable>
			void OnSubmittedWorkDone(CallbackMode mode, Callable callable)
			{
				// todo
			}
		};

		class CAGE_ENGINE_API ShaderModule : public GpuResource<ShaderModule>
		{
		public:
		};

		// configuration structs

		struct CAGE_ENGINE_API Extent3D
		{
			uint32 width = 0;
			uint32 height = 0;
			uint32 depthOrArrayLayers = 0;
		};

		struct CAGE_ENGINE_API BufferDescriptor
		{
			uintPtr size = 0;
			BufferUsage usage = BufferUsage::Undefined;
			StringView label;
		};

		struct CAGE_ENGINE_API TexelCopyTextureInfo
		{
			Texture texture;
			TextureAspect aspect = TextureAspect::Undefined;
			uint32 mipLevel = 0;
		};

		struct CAGE_ENGINE_API TexelCopyBufferLayout
		{
			uint32 bytesPerRow = 0;
			uint32 rowsPerImage = 0;
		};

		struct CAGE_ENGINE_API TexelCopyBufferInfo
		{
			TexelCopyBufferLayout layout;
			Buffer buffer;
		};

		struct CAGE_ENGINE_API TextureDescriptor
		{
			Extent3D size;
			TextureDimension dimension = TextureDimension::Undefined;
			sint32 mipLevelCount = 0;
			TextureFormat format = TextureFormat::Undefined;
			TextureUsage usage = TextureUsage::Undefined;
			StringView label;
		};

		struct CAGE_ENGINE_API TextureViewDescriptor
		{
			TextureViewDimension dimension = TextureViewDimension::Undefined;
			uint32 baseMipLevel = 0;
			uint32 mipLevelCount = 0;
			uint32 baseArrayLayer = 0;
			uint32 arrayLayerCount = 0;
			StringView label;
		};

		struct CAGE_ENGINE_API SamplerDescriptor
		{
			AddressMode addressModeU = AddressMode::Undefined;
			AddressMode addressModeV = AddressMode::Undefined;
			AddressMode addressModeW = AddressMode::Undefined;
			FilterMode magFilter = FilterMode::Undefined;
			FilterMode minFilter = FilterMode::Undefined;
			MipmapFilterMode mipmapFilter = MipmapFilterMode::Undefined;
			uint32 maxAnisotropy = 0;
			StringView label;
		};

		struct CAGE_ENGINE_API BindGroupLayoutEntry
		{
			uint32 binding = 0;
			ShaderStage visibility = ShaderStage::Undefined;

			struct BufferEntry
			{
				BufferBindingType type = BufferBindingType::Undefined;
				bool hasDynamicOffset = false;
			};
			BufferEntry buffer;

			struct TextureEntry
			{
				TextureSampleType sampleType = TextureSampleType::Undefined;
				TextureViewDimension viewDimension = TextureViewDimension::Undefined;
			};
			TextureEntry texture;

			struct SamplerEntry
			{
				SamplerBindingType type = SamplerBindingType::Undefined;
			};
			SamplerEntry sampler;
		};

		struct CAGE_ENGINE_API BindGroupLayoutDescriptor
		{
			PointerRange<const BindGroupLayoutEntry> entries;
			StringView label;
		};

		struct CAGE_ENGINE_API BindGroupEntry
		{
			uint32 binding = 0;
			uint32 offset = 0;
			uint32 size = 0; // use -1 to use the whole buffer
			Buffer buffer;
			TextureView textureView;
			Sampler sampler;
		};

		struct CAGE_ENGINE_API BindGroupDescriptor
		{
			BindGroupLayout layout;
			PointerRange<const BindGroupEntry> entries;
			StringView label;
		};

		struct CAGE_ENGINE_API CommandEncoderDescriptor
		{
			StringView label;
		};

		struct CAGE_ENGINE_API RenderPassColorAttachment
		{
			Vec4 clearValue;
			LoadOp loadOp = LoadOp::Undefined;
			StoreOp storeOp = StoreOp::Undefined;
			TextureView view;
		};

		struct CAGE_ENGINE_API RenderPassDepthStencilAttachment
		{
			TextureView view;
			LoadOp depthLoadOp = LoadOp::Undefined;
			StoreOp depthStoreOp = StoreOp::Undefined;
			Real depthClearValue;
		};

		struct CAGE_ENGINE_API RenderPassDescriptor
		{
			PointerRange<const RenderPassColorAttachment> colorAttachments;
			const RenderPassDepthStencilAttachment *depthStencilAttachment = nullptr;
			StringView label;
		};

		struct CAGE_ENGINE_API ShaderModuleDescriptor
		{
			PointerRange<const uint32> spirvCode;
			StringView label;
		};

		struct CAGE_ENGINE_API VertexAttribute
		{
			uint32 offset = 0;
			VertexFormat format = VertexFormat::Undefined;
			uint32 shaderLocation = 0;
		};

		struct CAGE_ENGINE_API VertexBufferLayout
		{
			uint32 arrayStride = 0;
			PointerRange<const VertexAttribute> attributes;
			VertexStepMode stepMode = VertexStepMode::Undefined;
		};

		struct CAGE_ENGINE_API BlendState
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

		struct CAGE_ENGINE_API DepthStencilState
		{
			TextureFormat format = TextureFormat::Undefined;
			CompareFunction depthCompare = CompareFunction::Undefined;
			bool depthWriteEnabled = false;
		};

		struct CAGE_ENGINE_API ColorTargetState
		{
			TextureFormat format = TextureFormat::Undefined;
			const BlendState *blend = nullptr;
		};

		struct CAGE_ENGINE_API FragmentState
		{
			ShaderModule module;
			PointerRange<const ColorTargetState> targets;
		};

		struct CAGE_ENGINE_API RenderPipelineDescriptor
		{
			struct VertexEntry
			{
				ShaderModule module;
				PointerRange<const VertexBufferLayout> buffers;
			};
			VertexEntry vertex;

			struct PrimitiveEntry
			{
				CullMode cullMode = CullMode::Undefined;
				PrimitiveTopology topology;
			};
			PrimitiveEntry primitive;

			const DepthStencilState *depthStencil = nullptr;
			const FragmentState *fragment = nullptr;
			RenderPipelineLayout layout;
		};

		struct CAGE_ENGINE_API PipelineLayoutDescriptor
		{
			PointerRange<const BindGroupLayout> bindGroupLayouts;
		};
	}

	CAGE_ENUM_BITS(gpu::BufferUsage);
	CAGE_ENUM_BITS(gpu::TextureUsage);
	CAGE_ENUM_BITS(gpu::ShaderStage);
}

#endif
