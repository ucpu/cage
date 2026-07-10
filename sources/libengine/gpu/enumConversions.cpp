#include "gpu.h"

namespace cage
{
	namespace gpuImpl
	{
		vk::ShaderStageFlags convertShaderStages(gpu::ShaderStagesFlags visibility)
		{
			vk::ShaderStageFlags r = {};
			if (any(visibility & gpu::ShaderStagesFlags::Vertex))
				r |= vk::ShaderStageFlagBits::eVertex;
			if (any(visibility & gpu::ShaderStagesFlags::Fragment))
				r |= vk::ShaderStageFlagBits::eFragment;
			if (any(visibility & gpu::ShaderStagesFlags::Compute))
				r |= vk::ShaderStageFlagBits::eCompute;
			return r;
		}

		vk::PrimitiveTopology convertPrimitiveTopology(gpu::PrimitiveTopologyEnum topology)
		{
			switch (topology)
			{
				case gpu::PrimitiveTopologyEnum::PointList:
					return vk::PrimitiveTopology::ePointList;
				case gpu::PrimitiveTopologyEnum::LineList:
					return vk::PrimitiveTopology::eLineList;
				case gpu::PrimitiveTopologyEnum::LineStrip:
					return vk::PrimitiveTopology::eLineStrip;
				case gpu::PrimitiveTopologyEnum::TriangleList:
					return vk::PrimitiveTopology::eTriangleList;
				case gpu::PrimitiveTopologyEnum::TriangleStrip:
					return vk::PrimitiveTopology::eTriangleStrip;
			}
			return vk::PrimitiveTopology::ePointList;
		}

		vk::CullModeFlags convertCullMode(gpu::CullModeEnum mode)
		{
			switch (mode)
			{
				case gpu::CullModeEnum::Back:
					return vk::CullModeFlagBits::eBack;
				case gpu::CullModeEnum::Front:
					return vk::CullModeFlagBits::eFront;
			}
			return vk::CullModeFlagBits::eNone;
		}

		vk::CompareOp convertCompareFunction(gpu::CompareFunctionEnum comp)
		{
			switch (comp)
			{
				case gpu::CompareFunctionEnum::Never:
					return vk::CompareOp::eNever;
				case gpu::CompareFunctionEnum::Less:
					return vk::CompareOp::eLess;
				case gpu::CompareFunctionEnum::Equal:
					return vk::CompareOp::eEqual;
				case gpu::CompareFunctionEnum::LessEqual:
					return vk::CompareOp::eLessOrEqual;
				case gpu::CompareFunctionEnum::Greater:
					return vk::CompareOp::eGreater;
				case gpu::CompareFunctionEnum::NotEqual:
					return vk::CompareOp::eNotEqual;
				case gpu::CompareFunctionEnum::GreaterEqual:
					return vk::CompareOp::eGreaterOrEqual;
				case gpu::CompareFunctionEnum::Always:
					return vk::CompareOp::eAlways;
			}
			return vk::CompareOp::eNever;
		}

		vk::VertexInputRate convertVertexStepMode(gpu::VertexStepModeEnum mode)
		{
			switch (mode)
			{
				case gpu::VertexStepModeEnum::Vertex:
					return vk::VertexInputRate::eVertex;
				case gpu::VertexStepModeEnum::Instance:
					return vk::VertexInputRate::eInstance;
			}
			return vk::VertexInputRate::eVertex;
		}

		vk::Format convertVertexFormat(gpu::VertexFormatEnum format)
		{
			switch (format)
			{
				case gpu::VertexFormatEnum::Uint8:
					return vk::Format::eR8Uint;
				case gpu::VertexFormatEnum::Uint8x2:
					return vk::Format::eR8G8Uint;
				case gpu::VertexFormatEnum::Uint8x4:
					return vk::Format::eR8G8B8A8Uint;
				case gpu::VertexFormatEnum::Sint8:
					return vk::Format::eR8Sint;
				case gpu::VertexFormatEnum::Sint8x2:
					return vk::Format::eR8G8Sint;
				case gpu::VertexFormatEnum::Sint8x4:
					return vk::Format::eR8G8B8A8Sint;
				case gpu::VertexFormatEnum::Unorm8:
					return vk::Format::eR8Unorm;
				case gpu::VertexFormatEnum::Unorm8x2:
					return vk::Format::eR8G8Unorm;
				case gpu::VertexFormatEnum::Unorm8x4:
					return vk::Format::eR8G8B8A8Unorm;
				case gpu::VertexFormatEnum::Unorm8x4BGRA:
					return vk::Format::eB8G8R8A8Unorm;
				case gpu::VertexFormatEnum::Snorm8:
					return vk::Format::eR8Snorm;
				case gpu::VertexFormatEnum::Snorm8x2:
					return vk::Format::eR8G8Snorm;
				case gpu::VertexFormatEnum::Snorm8x4:
					return vk::Format::eR8G8B8A8Snorm;
				case gpu::VertexFormatEnum::Uint16:
					return vk::Format::eR16Uint;
				case gpu::VertexFormatEnum::Uint16x2:
					return vk::Format::eR16G16Uint;
				case gpu::VertexFormatEnum::Uint16x4:
					return vk::Format::eR16G16B16A16Uint;
				case gpu::VertexFormatEnum::Sint16:
					return vk::Format::eR16Sint;
				case gpu::VertexFormatEnum::Sint16x2:
					return vk::Format::eR16G16Sint;
				case gpu::VertexFormatEnum::Sint16x4:
					return vk::Format::eR16G16B16A16Sint;
				case gpu::VertexFormatEnum::Unorm16:
					return vk::Format::eR16Unorm;
				case gpu::VertexFormatEnum::Unorm16x2:
					return vk::Format::eR16G16Unorm;
				case gpu::VertexFormatEnum::Unorm16x4:
					return vk::Format::eR16G16B16A16Unorm;
				case gpu::VertexFormatEnum::Snorm16:
					return vk::Format::eR16Snorm;
				case gpu::VertexFormatEnum::Snorm16x2:
					return vk::Format::eR16G16Snorm;
				case gpu::VertexFormatEnum::Snorm16x4:
					return vk::Format::eR16G16B16A16Snorm;
				case gpu::VertexFormatEnum::Float16:
					return vk::Format::eR16Sfloat;
				case gpu::VertexFormatEnum::Float16x2:
					return vk::Format::eR16G16Sfloat;
				case gpu::VertexFormatEnum::Float16x4:
					return vk::Format::eR16G16B16A16Sfloat;
				case gpu::VertexFormatEnum::Uint32:
					return vk::Format::eR32Uint;
				case gpu::VertexFormatEnum::Uint32x2:
					return vk::Format::eR32G32Uint;
				case gpu::VertexFormatEnum::Uint32x3:
					return vk::Format::eR32G32B32Uint;
				case gpu::VertexFormatEnum::Uint32x4:
					return vk::Format::eR32G32B32A32Uint;
				case gpu::VertexFormatEnum::Sint32:
					return vk::Format::eR32Sint;
				case gpu::VertexFormatEnum::Sint32x2:
					return vk::Format::eR32G32Sint;
				case gpu::VertexFormatEnum::Sint32x3:
					return vk::Format::eR32G32B32Sint;
				case gpu::VertexFormatEnum::Sint32x4:
					return vk::Format::eR32G32B32A32Sint;
				case gpu::VertexFormatEnum::Float32:
					return vk::Format::eR32Sfloat;
				case gpu::VertexFormatEnum::Float32x2:
					return vk::Format::eR32G32Sfloat;
				case gpu::VertexFormatEnum::Float32x3:
					return vk::Format::eR32G32B32Sfloat;
				case gpu::VertexFormatEnum::Float32x4:
					return vk::Format::eR32G32B32A32Sfloat;
				case gpu::VertexFormatEnum::Unorm10_10_10_2:
					return vk::Format::eA2B10G10R10UnormPack32;
			}
			return vk::Format::eUndefined;
		}

		vk::BlendFactor convertBlendFactor(gpu::BlendFactorEnum factor)
		{
			switch (factor)
			{
				case gpu::BlendFactorEnum::Zero:
					return vk::BlendFactor::eZero;
				case gpu::BlendFactorEnum::One:
					return vk::BlendFactor::eOne;
				case gpu::BlendFactorEnum::SrcColor:
					return vk::BlendFactor::eSrcColor;
				case gpu::BlendFactorEnum::OneMinusSrcColor:
					return vk::BlendFactor::eOneMinusSrcColor;
				case gpu::BlendFactorEnum::SrcAlpha:
					return vk::BlendFactor::eSrcAlpha;
				case gpu::BlendFactorEnum::OneMinusSrcAlpha:
					return vk::BlendFactor::eOneMinusSrcAlpha;
			}
			return vk::BlendFactor::eZero;
		}

		vk::BlendOp convertBlendOperation(gpu::BlendOperationEnum op)
		{
			switch (op)
			{
				case gpu::BlendOperationEnum::Add:
					return vk::BlendOp::eAdd;
				case gpu::BlendOperationEnum::Subtract:
					return vk::BlendOp::eSubtract;
				case gpu::BlendOperationEnum::ReverseSubtract:
					return vk::BlendOp::eReverseSubtract;
				case gpu::BlendOperationEnum::Min:
					return vk::BlendOp::eMin;
				case gpu::BlendOperationEnum::Max:
					return vk::BlendOp::eMax;
			}
			return vk::BlendOp::eAdd;
		}

		vk::Format convertTextureFormat(gpu::TextureFormatEnum format)
		{
			switch (format)
			{
				case gpu::TextureFormatEnum::R8Unorm:
					return vk::Format::eR8Unorm;
				case gpu::TextureFormatEnum::R8Snorm:
					return vk::Format::eR8Snorm;
				case gpu::TextureFormatEnum::R8Uint:
					return vk::Format::eR8Uint;
				case gpu::TextureFormatEnum::R8Sint:
					return vk::Format::eR8Sint;

				case gpu::TextureFormatEnum::R16Unorm:
					return vk::Format::eR16Unorm;
				case gpu::TextureFormatEnum::R16Snorm:
					return vk::Format::eR16Snorm;
				case gpu::TextureFormatEnum::R16Uint:
					return vk::Format::eR16Uint;
				case gpu::TextureFormatEnum::R16Sint:
					return vk::Format::eR16Sint;
				case gpu::TextureFormatEnum::R16Float:
					return vk::Format::eR16Sfloat;

				case gpu::TextureFormatEnum::RG8Unorm:
					return vk::Format::eR8G8Unorm;
				case gpu::TextureFormatEnum::RG8Snorm:
					return vk::Format::eR8G8Snorm;
				case gpu::TextureFormatEnum::RG8Uint:
					return vk::Format::eR8G8Uint;
				case gpu::TextureFormatEnum::RG8Sint:
					return vk::Format::eR8G8Sint;

				case gpu::TextureFormatEnum::R32Float:
					return vk::Format::eR32Sfloat;
				case gpu::TextureFormatEnum::R32Uint:
					return vk::Format::eR32Uint;
				case gpu::TextureFormatEnum::R32Sint:
					return vk::Format::eR32Sint;

				case gpu::TextureFormatEnum::RG16Unorm:
					return vk::Format::eR16G16Unorm;
				case gpu::TextureFormatEnum::RG16Snorm:
					return vk::Format::eR16G16Snorm;
				case gpu::TextureFormatEnum::RG16Uint:
					return vk::Format::eR16G16Uint;
				case gpu::TextureFormatEnum::RG16Sint:
					return vk::Format::eR16G16Sint;
				case gpu::TextureFormatEnum::RG16Float:
					return vk::Format::eR16G16Sfloat;

				case gpu::TextureFormatEnum::RGBA8Unorm:
					return vk::Format::eR8G8B8A8Unorm;
				case gpu::TextureFormatEnum::RGBA8UnormSrgb:
					return vk::Format::eR8G8B8A8Srgb;
				case gpu::TextureFormatEnum::RGBA8Snorm:
					return vk::Format::eR8G8B8A8Snorm;
				case gpu::TextureFormatEnum::RGBA8Uint:
					return vk::Format::eR8G8B8A8Uint;
				case gpu::TextureFormatEnum::RGBA8Sint:
					return vk::Format::eR8G8B8A8Sint;

				case gpu::TextureFormatEnum::BGRA8Unorm:
					return vk::Format::eB8G8R8A8Unorm;
				case gpu::TextureFormatEnum::BGRA8UnormSrgb:
					return vk::Format::eB8G8R8A8Srgb;

				case gpu::TextureFormatEnum::RGB10A2Uint:
					return vk::Format::eA2B10G10R10UintPack32;
				case gpu::TextureFormatEnum::RGB10A2Unorm:
					return vk::Format::eA2B10G10R10UnormPack32;

				case gpu::TextureFormatEnum::RG11B10Ufloat:
					return vk::Format::eB10G11R11UfloatPack32;
				case gpu::TextureFormatEnum::RGB9E5Ufloat:
					return vk::Format::eE5B9G9R9UfloatPack32;

				case gpu::TextureFormatEnum::RG32Float:
					return vk::Format::eR32G32Sfloat;
				case gpu::TextureFormatEnum::RG32Uint:
					return vk::Format::eR32G32Uint;
				case gpu::TextureFormatEnum::RG32Sint:
					return vk::Format::eR32G32Sint;

				case gpu::TextureFormatEnum::RGBA16Unorm:
					return vk::Format::eR16G16B16A16Unorm;
				case gpu::TextureFormatEnum::RGBA16Snorm:
					return vk::Format::eR16G16B16A16Snorm;
				case gpu::TextureFormatEnum::RGBA16Uint:
					return vk::Format::eR16G16B16A16Uint;
				case gpu::TextureFormatEnum::RGBA16Sint:
					return vk::Format::eR16G16B16A16Sint;
				case gpu::TextureFormatEnum::RGBA16Float:
					return vk::Format::eR16G16B16A16Sfloat;

				case gpu::TextureFormatEnum::RGBA32Float:
					return vk::Format::eR32G32B32A32Sfloat;
				case gpu::TextureFormatEnum::RGBA32Uint:
					return vk::Format::eR32G32B32A32Uint;
				case gpu::TextureFormatEnum::RGBA32Sint:
					return vk::Format::eR32G32B32A32Sint;

				case gpu::TextureFormatEnum::Stencil8:
					return vk::Format::eS8Uint;
				case gpu::TextureFormatEnum::Depth16Unorm:
					return vk::Format::eD16Unorm;
				case gpu::TextureFormatEnum::Depth32Float:
					return vk::Format::eD32Sfloat;
				case gpu::TextureFormatEnum::Depth32FloatStencil8:
					return vk::Format::eD32SfloatS8Uint;
				case gpu::TextureFormatEnum::Depth24Stencil8:
					return vk::Format::eD24UnormS8Uint;

				case gpu::TextureFormatEnum::BC1RGBAUnorm:
					return vk::Format::eBc1RgbaUnormBlock;
				case gpu::TextureFormatEnum::BC1RGBAUnormSrgb:
					return vk::Format::eBc1RgbaSrgbBlock;
				case gpu::TextureFormatEnum::BC2RGBAUnorm:
					return vk::Format::eBc2UnormBlock;
				case gpu::TextureFormatEnum::BC2RGBAUnormSrgb:
					return vk::Format::eBc2SrgbBlock;
				case gpu::TextureFormatEnum::BC3RGBAUnorm:
					return vk::Format::eBc3UnormBlock;
				case gpu::TextureFormatEnum::BC3RGBAUnormSrgb:
					return vk::Format::eBc3SrgbBlock;
				case gpu::TextureFormatEnum::BC4RUnorm:
					return vk::Format::eBc4UnormBlock;
				case gpu::TextureFormatEnum::BC4RSnorm:
					return vk::Format::eBc4SnormBlock;
				case gpu::TextureFormatEnum::BC5RGUnorm:
					return vk::Format::eBc5UnormBlock;
				case gpu::TextureFormatEnum::BC5RGSnorm:
					return vk::Format::eBc5SnormBlock;
				case gpu::TextureFormatEnum::BC6HRGBUfloat:
					return vk::Format::eBc6HUfloatBlock;
				case gpu::TextureFormatEnum::BC6HRGBFloat:
					return vk::Format::eBc6HSfloatBlock;
				case gpu::TextureFormatEnum::BC7RGBAUnorm:
					return vk::Format::eBc7UnormBlock;
				case gpu::TextureFormatEnum::BC7RGBAUnormSrgb:
					return vk::Format::eBc7SrgbBlock;

					// multi-planar video formats
					//case gpu::TextureFormatEnum::R8BG8Biplanar420Unorm:
					//	return vk::Format::eG8B8R82Plane420Unorm;
					//case gpu::TextureFormatEnum::R10X6BG10X6Biplanar420Unorm:
					//	return vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16;
					//case gpu::TextureFormatEnum::R8BG8A8Triplanar420Unorm:
					//	return vk::Format::eG8B8R83Plane420Unorm;
					//case gpu::TextureFormatEnum::R8BG8Biplanar422Unorm:
					//	return vk::Format::eG8B8R82Plane422Unorm;
					//case gpu::TextureFormatEnum::R8BG8Biplanar444Unorm:
					//	return vk::Format::eG8B8R82Plane444Unorm;
					//case gpu::TextureFormatEnum::R10X6BG10X6Biplanar422Unorm:
					//	return vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16;
					//case gpu::TextureFormatEnum::R10X6BG10X6Biplanar444Unorm:
					//	return vk::Format::eG10X6B10X6R10X62Plane444Unorm3Pack16;
			}

			return vk::Format::eUndefined;
		}
	}
}
