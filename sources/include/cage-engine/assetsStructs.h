#ifndef guard_assetsStructs_sdrz4gdftujaw
#define guard_assetsStructs_sdrz4gdftujaw

#include <array>

#include <cage-core/geometry.h>
#include <cage-engine/core.h>

namespace cage
{
	enum class MeshRenderFlags : uint32;

	struct CAGE_ENGINE_API MultiShaderHeader
	{
		uint32 customDataCount = 0; // number of floats passed from the game to the shader, per instance
		uint32 variantsCount = 0;

		// follows:
		// serialized vector of keywords, each StringBase<20>
		// for each variant:
		//   variant: uint32
		//   size: uint32
		//   serialized spirv
	};

	struct CAGE_ENGINE_API TextureHeader
	{
		TextureFlags flags = (TextureFlags)0;
		Vec3i resolution;
		uint32 channels = 0;
		uint32 mipLevels = 0;
		uint64 usage = 0; // wgpu::TextureUsage
		uint32 format = 0; // wgpu::TextureFormat
		uint32 sampleFilter = 0; // wgpu::FilterMode
		uint32 mipmapFilter = 0; // wgpu::MipmapFilterMode
		uint32 anisoFilter = 1;
		uint32 wrapX = 0; // wgpu::AddressMode
		uint32 wrapY = 0; // wgpu::AddressMode
		uint32 wrapZ = 0; // wgpu::AddressMode

		// follows:
		// for each mipmap level:
		//   resolution, Vec3i
		//   size, uint32
		//   array of bytes
	};

	struct CAGE_ENGINE_API ModelHeader
	{
		Mat4 importTransform;
		Aabb box;
		std::array<uint32, MaxTexturesCountPerMaterial> textureNames = {};
		uint32 shaderName = 0;
		MeshRenderFlags renderFlags = (MeshRenderFlags)0;
		sint32 renderLayer = 0;
		uint32 skeletonBones = 0;
		uint32 meshName = 0; // share geometry data (and collider) from the named model (this model contains no geometry)
		uint32 meshSize = 0; // bytes
		uint32 materialSize = 0; // bytes
		uint32 colliderSize = 0; // bytes

		// follows:
		// serialized mesh
		// material (may or may not be the MeshImportMaterial)
		// serialized collider (may be absent)
	};

	struct CAGE_ENGINE_API RenderObjectHeader
	{
		Vec3 color;
		Real intensity;
		Real opacity;
		uint32 skelAnimId = 0;

		Real worldSize;
		Real pixelsSize;
		uint32 lodsCount = 0;
		uint32 modelsCount = 0;

		// follows:
		// array of thresholds, each float
		// array of model indices, each uint32 (one item more than thresholds)
		// array of model names, each uint32
	};

	struct CAGE_ENGINE_API FontHeader
	{
		uint32 ftSize = 0; // bytes of font file
		uint32 imagesCount = 0;
		uint32 glyphsCount = 0;
		uint32 nominalSize = 0; // used in FT_Set_Pixel_Sizes
		Real nominalScale; // used to convert font units to 1pt
		Real lineOffset; // pixels for 1pt
		Real lineHeight; // pixels for 1pt

		struct CAGE_ENGINE_API GlyphData
		{
			Vec4 texUv;
			Vec2 bearing;
			Vec2 size;
			uint32 image = 0;
			uint32 glyphId = 0;
		};

		// follows:
		// font file
		// array of GlyphData
	};

	enum class SoundCompressionEnum : uint32
	{
		RawRaw, // short sounds
		CompressedRaw, // long sounds, but played many times concurrently
		CompressedCompressed, // long sounds, usually only played once at any given time
	};

	struct CAGE_ENGINE_API SoundSourceHeader
	{
		uint64 frames = 0;
		uint32 channels = 0;
		uint32 sampleRate = 0;
		SoundCompressionEnum soundType = SoundCompressionEnum::RawRaw;

		// follows (for raw file):
		// array of frames, each channels * float
		// ...

		// follows (for compressed file):
		// ogg/vorbis compressed stream
	};
}

#endif // guard_assetsStructs_sdrz4gdftujaw
