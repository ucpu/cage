#ifndef guard_gpuCore_erzike5
#define guard_gpuCore_erzike5

#include <vulkan/vulkan.hpp>

#include <cage-engine/core.h>

namespace cage
{
	namespace gpu
	{
		enum class BufferUsage
		{
			Undefined = 0,
			MapRead,
			CopyDst,
		};

		enum class TextureFormat
		{
			Undefined = 0,
			BC7RGBAUnormSrgb,
			BC4RUnorm,
			BC5RGUnorm,
			BC7RGBAUnorm,
			RGBA8UnormSrgb,
			R8Unorm,
			RG8Unorm,
			RGBA8Unorm,
		};

		enum class TextureUsage
		{
			Undefined = 0,
			CopyDst,
			TextureBinding,
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

		struct BindGroupLayout
		{};

		struct BindGroup
		{};

		struct VertexBufferLayout
		{};

		struct BufferDescriptor
		{};

		struct TexelCopyTextureInfo
		{};

		struct TexelCopyBufferInfo
		{};

		struct Extent3D
		{};

		struct Future;

		class Buffer;
		class Device;
		class Queue;
		class CommandBuffer;
		class RenderPipeline;
		class CommandEncoder;
		class RenderPassEncoder;
	}

	CAGE_ENUM_BITS(gpu::BufferUsage);
	CAGE_ENUM_BITS(gpu::TextureUsage);
}

#endif
