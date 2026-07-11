#ifndef guard_gpuCore_erzike5
#define guard_gpuCore_erzike5

#include <cage-engine/core.h>

namespace cage
{
	namespace gpu
	{
		///////////////////////////////////////////////////////////////////
		// enums
		///////////////////////////////////////////////////////////////////

		enum class AddressModeEnum
		{
			Undefined = 0,
			ClampToEdge,
			MirrorRepeat,
			Repeat,
		};

		enum class BlendFactorEnum
		{
			Undefined = 0,
			Zero,
			One,
			SrcColor,
			OneMinusSrcColor,
			SrcAlpha,
			OneMinusSrcAlpha,
			//Dst,
			//OneMinusDst,
			//DstAlpha,
			//OneMinusDstAlpha,
			//SrcAlphaSaturated,
			//Constant,
			//OneMinusConstant,
			//Src1,
			//OneMinusSrc1,
			//Src1Alpha,
			//OneMinusSrc1Alpha,
		};

		enum class BlendOperationEnum
		{
			Undefined = 0,
			Add,
			Subtract,
			ReverseSubtract,
			Min,
			Max,
		};

		enum class BufferBindingTypeEnum
		{
			Undefined = 0,
			ReadOnlyStorage,
			Storage,
			Uniform,
		};

		enum class BufferUsageFlags
		{
			Undefined = 0,
			CopyDst = 1u << 0,
			CopySrc = 1u << 1,
			Index = 1u << 2,
			Indirect = 1u << 3,
			MapRead = 1u << 4,
			MapWrite = 1u << 5,
			QueryResolve = 1u << 6,
			Storage = 1u << 7,
			TexelBuffer = 1u << 8,
			Uniform = 1u << 9,
			Vertex = 1u << 10,
		};

		enum class CallbackModeEnum
		{
			Undefined = 0,
			AllowProcessEvents,
			AllowSpontaneous,
			WaitAnyOnly,
		};

		enum class CompareFunctionEnum
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

		enum class CullModeEnum
		{
			Undefined = 0,
			None,
			Front,
			Back,
		};

		enum class FilterModeEnum
		{
			Undefined = 0,
			Linear,
			Nearest,
		};

		//enum class FrontFaceEnum
		//{
		//	Undefined = 0,
		//	CCW,
		//	CW,
		//};

		enum class IndexFormatEnum
		{
			Undefined = 0,
			Uint16,
			Uint32,
		};

		enum class LoadOpEnum
		{
			Undefined = 0,
			Load,
			Clear,
			//ExpandResolveTexture,
		};

		enum class MapModeEnum
		{
			Undefined = 0,
			Read,
			Write,
		};

		enum class PrimitiveTopologyEnum
		{
			Undefined = 0,
			PointList,
			LineList,
			LineStrip,
			TriangleList,
			TriangleStrip,
		};

		enum class SamplerBindingTypeEnum
		{
			Undefined = 0,
			Filtering,
			NonFiltering,
			//Comparison,
		};

		enum class ShaderStagesFlags
		{
			Undefined = 0,
			Fragment = 1u << 0,
			Vertex = 1u << 1,
			Compute = 1u << 2,
		};

		enum class StatusEnum
		{
			Undefined = 0,
			Success,
			Cancelled,
			TimedOut,
			Error,
		};

		enum class StoreOpEnum
		{
			Undefined = 0,
			Store,
			Discard,
		};

		enum class TextureAspectEnum
		{
			Undefined = 0,
			All,
			//StencilOnly,
			//DepthOnly,
			//Plane0Only,
			//Plane1Only,
			//Plane2Only,
		};

		enum class TextureDimensionEnum
		{
			Undefined = 0,
			Cube,
			CubeArray,
			e1D,
			e2D,
			e2DArray,
			e3D,
		};

		enum class TextureFormatEnum
		{
			Undefined = 0,
			R8Unorm,
			R8Snorm,
			R8Uint,
			R8Sint,
			R16Unorm,
			R16Snorm,
			R16Uint,
			R16Sint,
			R16Float,
			RG8Unorm,
			RG8Snorm,
			RG8Uint,
			RG8Sint,
			R32Float,
			R32Uint,
			R32Sint,
			RG16Unorm,
			RG16Snorm,
			RG16Uint,
			RG16Sint,
			RG16Float,
			RGBA8Unorm,
			RGBA8UnormSrgb,
			RGBA8Snorm,
			RGBA8Uint,
			RGBA8Sint,
			BGRA8Unorm,
			BGRA8UnormSrgb,
			RGB10A2Uint,
			RGB10A2Unorm,
			RG11B10Ufloat,
			RGB9E5Ufloat,
			RG32Float,
			RG32Uint,
			RG32Sint,
			RGBA16Unorm,
			RGBA16Snorm,
			RGBA16Uint,
			RGBA16Sint,
			RGBA16Float,
			RGBA32Float,
			RGBA32Uint,
			RGBA32Sint,
			Stencil8,
			Depth16Unorm,
			Depth24Stencil8,
			Depth32Float,
			Depth32FloatStencil8,
			BC1RGBAUnorm,
			BC1RGBAUnormSrgb,
			BC2RGBAUnorm,
			BC2RGBAUnormSrgb,
			BC3RGBAUnorm,
			BC3RGBAUnormSrgb,
			BC4RUnorm,
			BC4RSnorm,
			BC5RGUnorm,
			BC5RGSnorm,
			BC6HRGBUfloat,
			BC6HRGBFloat,
			BC7RGBAUnorm,
			BC7RGBAUnormSrgb,
			//R8BG8Biplanar420Unorm,
			//R10X6BG10X6Biplanar420Unorm,
			//R8BG8A8Triplanar420Unorm,
			//R8BG8Biplanar422Unorm,
			//R8BG8Biplanar444Unorm,
			//R10X6BG10X6Biplanar422Unorm,
			//R10X6BG10X6Biplanar444Unorm,
		};

		enum class TextureSampleTypeEnum
		{
			Undefined = 0,
			Float,
			UnfilterableFloat,
			//Depth,
			//Sint,
			//Uint,
		};

		enum class TextureUsageFlags
		{
			Undefined = 0,
			CopyDst = 1u << 0,
			CopySrc = 1u << 1,
			TextureBinding = 1u << 2,
			StorageBinding = 1u << 3,
			RenderAttachment = 1u << 4,
			TransientAttachment = 1u << 5,
		};

		enum class VertexFormatEnum
		{
			Undefined = 0,
			Float16,
			Float16x2,
			Float16x4,
			Float32,
			Float32x2,
			Float32x3,
			Float32x4,
			Sint16,
			Sint16x2,
			Sint16x4,
			Sint32,
			Sint32x2,
			Sint32x3,
			Sint32x4,
			Sint8,
			Sint8x2,
			Sint8x4,
			Snorm16,
			Snorm16x2,
			Snorm16x4,
			Snorm8,
			Snorm8x2,
			Snorm8x4,
			Uint16,
			Uint16x2,
			Uint16x4,
			Uint32,
			Uint32x2,
			Uint32x3,
			Uint32x4,
			Uint8,
			Uint8x2,
			Uint8x4,
			Unorm10_10_10_2,
			Unorm16,
			Unorm16x2,
			Unorm16x4,
			Unorm8,
			Unorm8x2,
			Unorm8x4,
			Unorm8x4BGRA,
		};

		enum class VertexStepModeEnum
		{
			Undefined = 0,
			Vertex,
			Instance,
		};

		///////////////////////////////////////////////////////////////////
		// forward declarations
		///////////////////////////////////////////////////////////////////

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

		struct BindGroupDescriptor;
		struct BindGroupLayoutDescriptor;
		struct BufferDescriptor;
		struct CommandEncoderDescriptor;
		struct PipelineLayoutDescriptor;
		struct RenderPassDescriptor;
		struct RenderPipelineDescriptor;
		struct SamplerDescriptor;
		struct ShaderModuleDescriptor;
		struct TexelCopyBufferInfo;
		struct TexelCopyBufferLayout;
		struct TexelCopyTextureInfo;
		struct TextureDescriptor;
		struct TextureViewDescriptor;
		struct VertexBufferLayout;
	}

	CAGE_ENUM_BITS(gpu::BufferUsageFlags);
	CAGE_ENUM_BITS(gpu::ShaderStagesFlags);
	CAGE_ENUM_BITS(gpu::TextureUsageFlags);
}

#endif
