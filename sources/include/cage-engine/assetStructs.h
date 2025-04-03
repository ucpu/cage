#ifndef guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_
#define guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_

#include <cage-core/geometry.h>
#include <cage-engine/core.h>

namespace cage
{
	enum class MeshRenderFlags : uint32;

	struct CAGE_ENGINE_API ShaderProgramHeader
	{
		uint32 customDataCount = 0; // number of floats passed from the game to the shader, per instance
		uint32 keywordsCount = 0;
		uint32 stagesCount = 0;

		// follows:
		// array of keywords, each StringBase<20>
		// for each stage:
		//   type, uint32
		//   length, uint32
		//   code, array of chars
	};

	enum class TextureFlags : uint32
	{
		None = 0,
		Compressed = 1 << 0,
		GenerateMipmaps = 1 << 1,
		Srgb = 1 << 2,
	};

	enum class TextureSwizzleEnum : uint8
	{
		Zero,
		One,
		R,
		G,
		B,
		A,
	};

	struct CAGE_ENGINE_API TextureHeader
	{
		TextureFlags flags = TextureFlags::None;
		uint32 target = 0; // GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP, ...
		Vec3i resolution;
		uint32 mipmapLevels = 0; // number of levels that the texture will have
		uint32 containedLevels = 0; // number of levels that the asset contains
		uint32 channels = 0;
		uint32 internalFormat = 0;
		uint32 copyFormat = 0;
		uint32 copyType = 0;
		uint32 filterMin = 0;
		uint32 filterMag = 0;
		uint32 filterAniso = 0;
		uint32 wrapX = 0;
		uint32 wrapY = 0;
		uint32 wrapZ = 0;
		TextureSwizzleEnum swizzle[4] = {};

		// follows:
		// for each mipmap level:
		//   resolution, Vec3i
		//   size, uint32
		//   array of texels
	};

	struct CAGE_ENGINE_API ModelHeader
	{
		Mat4 importTransform;
		Aabb box;
		uint32 textureNames[MaxTexturesCountPerMaterial] = {};
		uint32 shaderName = 0;
		MeshRenderFlags renderFlags = (MeshRenderFlags)0;
		sint32 renderLayer = 0;
		uint32 skeletonBones = 0;
		uint32 materialSize = 0; // bytes

		// follows:
		// material (may or may not be the MeshImportMaterial)
		// serialized mesh
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

#endif // guard_assets_h_5aade310_996b_42c7_8684_2100f6625d36_
