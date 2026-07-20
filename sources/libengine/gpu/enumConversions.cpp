#include "gpu.h"

namespace cage
{
	namespace gpu
	{
		vk::AttachmentLoadOp convertLoadOperation(LoadOpEnum op)
		{
			CAGE_ASSERT(op != LoadOpEnum::Undefined);
			switch (op)
			{
				case LoadOpEnum::Clear:
					return vk::AttachmentLoadOp::eClear;
				case LoadOpEnum::Load:
					return vk::AttachmentLoadOp::eLoad;
			}
			return vk::AttachmentLoadOp::eNoneKHR;
		}

		vk::AttachmentStoreOp convertStoreOperation(StoreOpEnum op)
		{
			CAGE_ASSERT(op != StoreOpEnum::Undefined);
			switch (op)
			{
				case StoreOpEnum::Discard:
					return vk::AttachmentStoreOp::eDontCare;
				case StoreOpEnum::Store:
					return vk::AttachmentStoreOp::eStore;
			}
			return vk::AttachmentStoreOp::eNoneKHR;
		}

		vk::BlendFactor convertBlendFactor(BlendFactorEnum factor)
		{
			CAGE_ASSERT(factor != BlendFactorEnum::Undefined);
			switch (factor)
			{
				case BlendFactorEnum::Zero:
					return vk::BlendFactor::eZero;
				case BlendFactorEnum::One:
					return vk::BlendFactor::eOne;
				case BlendFactorEnum::SrcColor:
					return vk::BlendFactor::eSrcColor;
				case BlendFactorEnum::OneMinusSrcColor:
					return vk::BlendFactor::eOneMinusSrcColor;
				case BlendFactorEnum::SrcAlpha:
					return vk::BlendFactor::eSrcAlpha;
				case BlendFactorEnum::OneMinusSrcAlpha:
					return vk::BlendFactor::eOneMinusSrcAlpha;
			}
			return vk::BlendFactor::eZero;
		}

		vk::BlendOp convertBlendOperation(BlendOperationEnum op)
		{
			CAGE_ASSERT(op != BlendOperationEnum::Undefined);
			switch (op)
			{
				case BlendOperationEnum::Add:
					return vk::BlendOp::eAdd;
				case BlendOperationEnum::Subtract:
					return vk::BlendOp::eSubtract;
				case BlendOperationEnum::ReverseSubtract:
					return vk::BlendOp::eReverseSubtract;
				case BlendOperationEnum::Min:
					return vk::BlendOp::eMin;
				case BlendOperationEnum::Max:
					return vk::BlendOp::eMax;
			}
			return vk::BlendOp::eAdd;
		}

		vk::BufferUsageFlags convertBufferUsage(BufferUsageFlags flags)
		{
			CAGE_ASSERT(flags != BufferUsageFlags::Undefined);
			vk::BufferUsageFlags bits = {};
			if (any(flags & BufferUsageFlags::CopyDst))
				bits |= vk::BufferUsageFlagBits::eTransferDst;
			if (any(flags & BufferUsageFlags::CopySrc))
				bits |= vk::BufferUsageFlagBits::eTransferSrc;
			if (any(flags & BufferUsageFlags::Index))
				bits |= vk::BufferUsageFlagBits::eIndexBuffer;
			if (any(flags & BufferUsageFlags::Indirect))
				bits |= vk::BufferUsageFlagBits::eIndirectBuffer;
			//if (any(flags & BufferUsageFlags::QueryResolve))
			//	bits |= vk::BufferUsageFlagBits::;
			if (any(flags & BufferUsageFlags::Storage))
				bits |= vk::BufferUsageFlagBits::eStorageBuffer;
			if (any(flags & BufferUsageFlags::TexelBuffer))
				bits |= vk::BufferUsageFlagBits::eStorageTexelBuffer | vk::BufferUsageFlagBits::eUniformTexelBuffer;
			if (any(flags & BufferUsageFlags::Uniform))
				bits |= vk::BufferUsageFlagBits::eUniformBuffer;
			if (any(flags & BufferUsageFlags::Vertex))
				bits |= vk::BufferUsageFlagBits::eVertexBuffer;
			return bits;
		}

		vk::CompareOp convertCompareFunction(CompareFunctionEnum comp)
		{
			CAGE_ASSERT(comp != CompareFunctionEnum::Undefined);
			switch (comp)
			{
				case CompareFunctionEnum::Never:
					return vk::CompareOp::eNever;
				case CompareFunctionEnum::Less:
					return vk::CompareOp::eLess;
				case CompareFunctionEnum::Equal:
					return vk::CompareOp::eEqual;
				case CompareFunctionEnum::LessEqual:
					return vk::CompareOp::eLessOrEqual;
				case CompareFunctionEnum::Greater:
					return vk::CompareOp::eGreater;
				case CompareFunctionEnum::NotEqual:
					return vk::CompareOp::eNotEqual;
				case CompareFunctionEnum::GreaterEqual:
					return vk::CompareOp::eGreaterOrEqual;
				case CompareFunctionEnum::Always:
					return vk::CompareOp::eAlways;
			}
			return vk::CompareOp::eNever;
		}

		vk::CullModeFlags convertCullMode(CullModeEnum mode)
		{
			CAGE_ASSERT(mode != CullModeEnum::Undefined);
			switch (mode)
			{
				case CullModeEnum::Back:
					return vk::CullModeFlagBits::eBack;
				case CullModeEnum::Front:
					return vk::CullModeFlagBits::eFront;
			}
			return vk::CullModeFlagBits::eNone;
		}

		vk::Filter convertFilter(FilterModeEnum filter)
		{
			//CAGE_ASSERT(filter != FilterModeEnum::Undefined);
			switch (filter)
			{
				case FilterModeEnum::Nearest:
					return vk::Filter::eNearest;
				case FilterModeEnum::Linear:
					return vk::Filter::eLinear;
			}
			return vk::Filter::eNearest;
		}

		vk::Format convertTextureFormat(TextureFormatEnum format)
		{
			CAGE_ASSERT(format != TextureFormatEnum::Undefined);
			switch (format)
			{
				case TextureFormatEnum::R8Unorm:
					return vk::Format::eR8Unorm;
				case TextureFormatEnum::R8Snorm:
					return vk::Format::eR8Snorm;
				case TextureFormatEnum::R8Uint:
					return vk::Format::eR8Uint;
				case TextureFormatEnum::R8Sint:
					return vk::Format::eR8Sint;

				case TextureFormatEnum::R16Unorm:
					return vk::Format::eR16Unorm;
				case TextureFormatEnum::R16Snorm:
					return vk::Format::eR16Snorm;
				case TextureFormatEnum::R16Uint:
					return vk::Format::eR16Uint;
				case TextureFormatEnum::R16Sint:
					return vk::Format::eR16Sint;
				case TextureFormatEnum::R16Float:
					return vk::Format::eR16Sfloat;

				case TextureFormatEnum::RG8Unorm:
					return vk::Format::eR8G8Unorm;
				case TextureFormatEnum::RG8Snorm:
					return vk::Format::eR8G8Snorm;
				case TextureFormatEnum::RG8Uint:
					return vk::Format::eR8G8Uint;
				case TextureFormatEnum::RG8Sint:
					return vk::Format::eR8G8Sint;

				case TextureFormatEnum::R32Float:
					return vk::Format::eR32Sfloat;
				case TextureFormatEnum::R32Uint:
					return vk::Format::eR32Uint;
				case TextureFormatEnum::R32Sint:
					return vk::Format::eR32Sint;

				case TextureFormatEnum::RG16Unorm:
					return vk::Format::eR16G16Unorm;
				case TextureFormatEnum::RG16Snorm:
					return vk::Format::eR16G16Snorm;
				case TextureFormatEnum::RG16Uint:
					return vk::Format::eR16G16Uint;
				case TextureFormatEnum::RG16Sint:
					return vk::Format::eR16G16Sint;
				case TextureFormatEnum::RG16Float:
					return vk::Format::eR16G16Sfloat;

				case TextureFormatEnum::RGBA8Unorm:
					return vk::Format::eR8G8B8A8Unorm;
				case TextureFormatEnum::RGBA8UnormSrgb:
					return vk::Format::eR8G8B8A8Srgb;
				case TextureFormatEnum::RGBA8Snorm:
					return vk::Format::eR8G8B8A8Snorm;
				case TextureFormatEnum::RGBA8Uint:
					return vk::Format::eR8G8B8A8Uint;
				case TextureFormatEnum::RGBA8Sint:
					return vk::Format::eR8G8B8A8Sint;

				case TextureFormatEnum::BGRA8Unorm:
					return vk::Format::eB8G8R8A8Unorm;
				case TextureFormatEnum::BGRA8UnormSrgb:
					return vk::Format::eB8G8R8A8Srgb;

				case TextureFormatEnum::RGB10A2Uint:
					return vk::Format::eA2B10G10R10UintPack32;
				case TextureFormatEnum::RGB10A2Unorm:
					return vk::Format::eA2B10G10R10UnormPack32;

				case TextureFormatEnum::RG11B10Ufloat:
					return vk::Format::eB10G11R11UfloatPack32;
				case TextureFormatEnum::RGB9E5Ufloat:
					return vk::Format::eE5B9G9R9UfloatPack32;

				case TextureFormatEnum::RG32Float:
					return vk::Format::eR32G32Sfloat;
				case TextureFormatEnum::RG32Uint:
					return vk::Format::eR32G32Uint;
				case TextureFormatEnum::RG32Sint:
					return vk::Format::eR32G32Sint;

				case TextureFormatEnum::RGBA16Unorm:
					return vk::Format::eR16G16B16A16Unorm;
				case TextureFormatEnum::RGBA16Snorm:
					return vk::Format::eR16G16B16A16Snorm;
				case TextureFormatEnum::RGBA16Uint:
					return vk::Format::eR16G16B16A16Uint;
				case TextureFormatEnum::RGBA16Sint:
					return vk::Format::eR16G16B16A16Sint;
				case TextureFormatEnum::RGBA16Float:
					return vk::Format::eR16G16B16A16Sfloat;

				case TextureFormatEnum::RGBA32Float:
					return vk::Format::eR32G32B32A32Sfloat;
				case TextureFormatEnum::RGBA32Uint:
					return vk::Format::eR32G32B32A32Uint;
				case TextureFormatEnum::RGBA32Sint:
					return vk::Format::eR32G32B32A32Sint;

				case TextureFormatEnum::Stencil8:
					return vk::Format::eS8Uint;
				case TextureFormatEnum::Depth16Unorm:
					return vk::Format::eD16Unorm;
				case TextureFormatEnum::Depth32Float:
					return vk::Format::eD32Sfloat;
				case TextureFormatEnum::Depth32FloatStencil8:
					return vk::Format::eD32SfloatS8Uint;
				case TextureFormatEnum::Depth24Stencil8:
					return vk::Format::eD24UnormS8Uint;

				case TextureFormatEnum::BC1RGBAUnorm:
					return vk::Format::eBc1RgbaUnormBlock;
				case TextureFormatEnum::BC1RGBAUnormSrgb:
					return vk::Format::eBc1RgbaSrgbBlock;
				case TextureFormatEnum::BC2RGBAUnorm:
					return vk::Format::eBc2UnormBlock;
				case TextureFormatEnum::BC2RGBAUnormSrgb:
					return vk::Format::eBc2SrgbBlock;
				case TextureFormatEnum::BC3RGBAUnorm:
					return vk::Format::eBc3UnormBlock;
				case TextureFormatEnum::BC3RGBAUnormSrgb:
					return vk::Format::eBc3SrgbBlock;
				case TextureFormatEnum::BC4RUnorm:
					return vk::Format::eBc4UnormBlock;
				case TextureFormatEnum::BC4RSnorm:
					return vk::Format::eBc4SnormBlock;
				case TextureFormatEnum::BC5RGUnorm:
					return vk::Format::eBc5UnormBlock;
				case TextureFormatEnum::BC5RGSnorm:
					return vk::Format::eBc5SnormBlock;
				case TextureFormatEnum::BC6HRGBUfloat:
					return vk::Format::eBc6HUfloatBlock;
				case TextureFormatEnum::BC6HRGBFloat:
					return vk::Format::eBc6HSfloatBlock;
				case TextureFormatEnum::BC7RGBAUnorm:
					return vk::Format::eBc7UnormBlock;
				case TextureFormatEnum::BC7RGBAUnormSrgb:
					return vk::Format::eBc7SrgbBlock;

					// multi-planar video formats
					//case TextureFormatEnum::R8BG8Biplanar420Unorm:
					//	return vk::Format::eG8B8R82Plane420Unorm;
					//case TextureFormatEnum::R10X6BG10X6Biplanar420Unorm:
					//	return vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16;
					//case TextureFormatEnum::R8BG8A8Triplanar420Unorm:
					//	return vk::Format::eG8B8R83Plane420Unorm;
					//case TextureFormatEnum::R8BG8Biplanar422Unorm:
					//	return vk::Format::eG8B8R82Plane422Unorm;
					//case TextureFormatEnum::R8BG8Biplanar444Unorm:
					//	return vk::Format::eG8B8R82Plane444Unorm;
					//case TextureFormatEnum::R10X6BG10X6Biplanar422Unorm:
					//	return vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16;
					//case TextureFormatEnum::R10X6BG10X6Biplanar444Unorm:
					//	return vk::Format::eG10X6B10X6R10X62Plane444Unorm3Pack16;
			}

			return vk::Format::eUndefined;
		}

		vk::Format convertVertexFormat(VertexFormatEnum format)
		{
			CAGE_ASSERT(format != VertexFormatEnum::Undefined);
			switch (format)
			{
				case VertexFormatEnum::Uint8:
					return vk::Format::eR8Uint;
				case VertexFormatEnum::Uint8x2:
					return vk::Format::eR8G8Uint;
				case VertexFormatEnum::Uint8x4:
					return vk::Format::eR8G8B8A8Uint;
				case VertexFormatEnum::Sint8:
					return vk::Format::eR8Sint;
				case VertexFormatEnum::Sint8x2:
					return vk::Format::eR8G8Sint;
				case VertexFormatEnum::Sint8x4:
					return vk::Format::eR8G8B8A8Sint;
				case VertexFormatEnum::Unorm8:
					return vk::Format::eR8Unorm;
				case VertexFormatEnum::Unorm8x2:
					return vk::Format::eR8G8Unorm;
				case VertexFormatEnum::Unorm8x4:
					return vk::Format::eR8G8B8A8Unorm;
				case VertexFormatEnum::Unorm8x4BGRA:
					return vk::Format::eB8G8R8A8Unorm;
				case VertexFormatEnum::Snorm8:
					return vk::Format::eR8Snorm;
				case VertexFormatEnum::Snorm8x2:
					return vk::Format::eR8G8Snorm;
				case VertexFormatEnum::Snorm8x4:
					return vk::Format::eR8G8B8A8Snorm;
				case VertexFormatEnum::Uint16:
					return vk::Format::eR16Uint;
				case VertexFormatEnum::Uint16x2:
					return vk::Format::eR16G16Uint;
				case VertexFormatEnum::Uint16x4:
					return vk::Format::eR16G16B16A16Uint;
				case VertexFormatEnum::Sint16:
					return vk::Format::eR16Sint;
				case VertexFormatEnum::Sint16x2:
					return vk::Format::eR16G16Sint;
				case VertexFormatEnum::Sint16x4:
					return vk::Format::eR16G16B16A16Sint;
				case VertexFormatEnum::Unorm16:
					return vk::Format::eR16Unorm;
				case VertexFormatEnum::Unorm16x2:
					return vk::Format::eR16G16Unorm;
				case VertexFormatEnum::Unorm16x4:
					return vk::Format::eR16G16B16A16Unorm;
				case VertexFormatEnum::Snorm16:
					return vk::Format::eR16Snorm;
				case VertexFormatEnum::Snorm16x2:
					return vk::Format::eR16G16Snorm;
				case VertexFormatEnum::Snorm16x4:
					return vk::Format::eR16G16B16A16Snorm;
				case VertexFormatEnum::Float16:
					return vk::Format::eR16Sfloat;
				case VertexFormatEnum::Float16x2:
					return vk::Format::eR16G16Sfloat;
				case VertexFormatEnum::Float16x4:
					return vk::Format::eR16G16B16A16Sfloat;
				case VertexFormatEnum::Uint32:
					return vk::Format::eR32Uint;
				case VertexFormatEnum::Uint32x2:
					return vk::Format::eR32G32Uint;
				case VertexFormatEnum::Uint32x3:
					return vk::Format::eR32G32B32Uint;
				case VertexFormatEnum::Uint32x4:
					return vk::Format::eR32G32B32A32Uint;
				case VertexFormatEnum::Sint32:
					return vk::Format::eR32Sint;
				case VertexFormatEnum::Sint32x2:
					return vk::Format::eR32G32Sint;
				case VertexFormatEnum::Sint32x3:
					return vk::Format::eR32G32B32Sint;
				case VertexFormatEnum::Sint32x4:
					return vk::Format::eR32G32B32A32Sint;
				case VertexFormatEnum::Float32:
					return vk::Format::eR32Sfloat;
				case VertexFormatEnum::Float32x2:
					return vk::Format::eR32G32Sfloat;
				case VertexFormatEnum::Float32x3:
					return vk::Format::eR32G32B32Sfloat;
				case VertexFormatEnum::Float32x4:
					return vk::Format::eR32G32B32A32Sfloat;
				case VertexFormatEnum::Unorm10_10_10_2:
					return vk::Format::eA2B10G10R10UnormPack32;
			}
			return vk::Format::eUndefined;
		}

		vk::FrontFace convertFrontFace(FrontFaceEnum face)
		{
			CAGE_ASSERT(face != FrontFaceEnum::Undefined);
			switch (face)
			{
				case FrontFaceEnum::CCW:
					return vk::FrontFace::eCounterClockwise;
				case FrontFaceEnum::CW:
					return vk::FrontFace::eClockwise;
			}
			return vk::FrontFace::eCounterClockwise;
		}

		vk::ImageAspectFlags convertAspectMask(TextureFormatEnum format)
		{
			CAGE_ASSERT(format != TextureFormatEnum::Undefined);
			switch (format)
			{
				case TextureFormatEnum::Stencil8:
					return vk::ImageAspectFlagBits::eStencil;
				case TextureFormatEnum::Depth16Unorm:
				case TextureFormatEnum::Depth32Float:
					return vk::ImageAspectFlagBits::eDepth;
				case TextureFormatEnum::Depth32FloatStencil8:
				case TextureFormatEnum::Depth24Stencil8:
					return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
				default:
					return vk::ImageAspectFlagBits::eColor;
			}
		}

		vk::ImageUsageFlags convertTextureUsage(TextureUsageFlags flags, TextureFormatEnum format)
		{
			CAGE_ASSERT(flags != TextureUsageFlags::Undefined);
			CAGE_ASSERT(format != TextureFormatEnum::Undefined);
			vk::ImageUsageFlags bits = {};
			if (any(flags & TextureUsageFlags::CopyDst))
				bits |= vk::ImageUsageFlagBits::eTransferDst;
			if (any(flags & TextureUsageFlags::CopySrc))
				bits |= vk::ImageUsageFlagBits::eTransferSrc;
			if (any(flags & TextureUsageFlags::TextureBinding))
				bits |= vk::ImageUsageFlagBits::eSampled;
			if (any(flags & TextureUsageFlags::StorageBinding))
				bits |= vk::ImageUsageFlagBits::eStorage;
			if (any(flags & TextureUsageFlags::RenderAttachment))
			{
				switch (format)
				{
					case TextureFormatEnum::Stencil8:
					case TextureFormatEnum::Depth16Unorm:
					case TextureFormatEnum::Depth32Float:
					case TextureFormatEnum::Depth32FloatStencil8:
					case TextureFormatEnum::Depth24Stencil8:
						bits |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
						break;
					default:
						bits |= vk::ImageUsageFlagBits::eColorAttachment;
						break;
				}
			}
			if (any(flags & TextureUsageFlags::TransientAttachment))
				bits |= vk::ImageUsageFlagBits::eTransientAttachment;
			return bits;
		}

		vk::IndexType convertIndexFormat(IndexFormatEnum format)
		{
			CAGE_ASSERT(format != IndexFormatEnum::Undefined);
			switch (format)
			{
				case IndexFormatEnum::Uint16:
					return vk::IndexType::eUint16;
				case IndexFormatEnum::Uint32:
					return vk::IndexType::eUint32;
			}
			return vk::IndexType::eNoneKHR;
		}

		vk::PrimitiveTopology convertPrimitiveTopology(PrimitiveTopologyEnum topology)
		{
			CAGE_ASSERT(topology != PrimitiveTopologyEnum::Undefined);
			switch (topology)
			{
				case PrimitiveTopologyEnum::PointList:
					return vk::PrimitiveTopology::ePointList;
				case PrimitiveTopologyEnum::LineList:
					return vk::PrimitiveTopology::eLineList;
				case PrimitiveTopologyEnum::LineStrip:
					return vk::PrimitiveTopology::eLineStrip;
				case PrimitiveTopologyEnum::TriangleList:
					return vk::PrimitiveTopology::eTriangleList;
				case PrimitiveTopologyEnum::TriangleStrip:
					return vk::PrimitiveTopology::eTriangleStrip;
			}
			return vk::PrimitiveTopology::ePointList;
		}

		vk::SamplerAddressMode convertAddressMode(AddressModeEnum mode)
		{
			//CAGE_ASSERT(mode != AddressModeEnum::Undefined);
			switch (mode)
			{
				case AddressModeEnum::ClampToEdge:
					return vk::SamplerAddressMode::eClampToEdge;
				case AddressModeEnum::Repeat:
					return vk::SamplerAddressMode::eRepeat;
				case AddressModeEnum::MirrorRepeat:
					return vk::SamplerAddressMode::eMirroredRepeat;
			}
			return vk::SamplerAddressMode::eClampToEdge;
		}

		vk::SamplerMipmapMode convertMipmapFilter(FilterModeEnum filter)
		{
			//CAGE_ASSERT(filter != FilterModeEnum::Undefined);
			switch (filter)
			{
				case FilterModeEnum::Nearest:
					return vk::SamplerMipmapMode::eNearest;
				case FilterModeEnum::Linear:
					return vk::SamplerMipmapMode::eLinear;
			}
			return vk::SamplerMipmapMode::eNearest;
		}

		vk::ShaderStageFlags convertShaderStages(ShaderStagesFlags visibility)
		{
			CAGE_ASSERT(visibility != ShaderStagesFlags::Undefined);
			vk::ShaderStageFlags r = {};
			if (any(visibility & ShaderStagesFlags::Vertex))
				r |= vk::ShaderStageFlagBits::eVertex;
			if (any(visibility & ShaderStagesFlags::Fragment))
				r |= vk::ShaderStageFlagBits::eFragment;
			if (any(visibility & ShaderStagesFlags::Compute))
				r |= vk::ShaderStageFlagBits::eCompute;
			return r;
		}

		vk::DescriptorType convertBindingBufferType(BufferBindingTypeEnum type, bool hasDynamicOffset)
		{
			CAGE_ASSERT(type != BufferBindingTypeEnum::Undefined);
			switch (type)
			{
				case BufferBindingTypeEnum::Uniform:
					return hasDynamicOffset ? vk::DescriptorType::eUniformBufferDynamic : vk::DescriptorType::eUniformBuffer;
				case BufferBindingTypeEnum::Storage:
				case BufferBindingTypeEnum::ReadOnlyStorage:
					return hasDynamicOffset ? vk::DescriptorType::eStorageBufferDynamic : vk::DescriptorType::eStorageBuffer;
			}
			return {};
		}

		void assignImageStateBarrierFlags(ImageStateEnum state, vk::PipelineStageFlags2 &stageMask, vk::AccessFlags2 &accessMask, vk::ImageLayout &imageLayout)
		{
			switch (state)
			{
				default:
				case ImageStateEnum::Undefined:
					stageMask = vk::PipelineStageFlagBits2::eNone;
					accessMask = vk::AccessFlagBits2::eNone;
					imageLayout = vk::ImageLayout::eUndefined;
					break;
				case ImageStateEnum::ColorAttachment:
					stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
					accessMask = vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite;
					imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
					break;
				case ImageStateEnum::DepthAttachment:
					stageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
					accessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
					imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					break;
				case ImageStateEnum::Sampled:
					stageMask = vk::PipelineStageFlagBits2::eFragmentShader;
					accessMask = vk::AccessFlagBits2::eShaderSampledRead;
					imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
					break;
				case ImageStateEnum::StorageRead:
					stageMask = vk::PipelineStageFlagBits2::eComputeShader;
					accessMask = vk::AccessFlagBits2::eShaderStorageRead;
					imageLayout = vk::ImageLayout::eGeneral;
					break;
				case ImageStateEnum::StorageWrite:
					stageMask = vk::PipelineStageFlagBits2::eComputeShader;
					accessMask = vk::AccessFlagBits2::eShaderStorageWrite;
					imageLayout = vk::ImageLayout::eGeneral;
					break;
				case ImageStateEnum::TransferSrc:
					stageMask = vk::PipelineStageFlagBits2::eTransfer;
					accessMask = vk::AccessFlagBits2::eTransferRead;
					imageLayout = vk::ImageLayout::eTransferSrcOptimal;
					break;
				case ImageStateEnum::TransferDst:
					stageMask = vk::PipelineStageFlagBits2::eTransfer;
					accessMask = vk::AccessFlagBits2::eTransferWrite;
					imageLayout = vk::ImageLayout::eTransferDstOptimal;
					break;
				case ImageStateEnum::Present:
					stageMask = vk::PipelineStageFlagBits2::eNone;
					accessMask = vk::AccessFlagBits2::eNone;
					imageLayout = vk::ImageLayout::ePresentSrcKHR;
					break;
			}
		}
	}
}
