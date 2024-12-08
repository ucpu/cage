#ifndef guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_
#define guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_

#include <cage-core/geometry.h>
#include <cage-engine/core.h>

namespace cage
{
	enum class MeshRenderFlags : uint32;

	/*
	struct CAGE_ENGINE_API ShaderProgramHeader
	{
		// follows:
		// number of keywords, uint32
		// array of keywords, each StringBase<20>
		// number of stages, uint32
		// for each stage:
		//   stage name, uint32
		//   length, uint32
		//   stage code, array of chars
	};
	*/

	enum class TextureFlags : uint32
	{
		None = 0,
		GenerateMipmaps = 1 << 0,
		AnimationLoop = 1 << 1,
		Srgb = 1 << 2,
		Compressed = 1 << 3,
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
		uint64 animationDuration;
		TextureFlags flags;
		uint32 target; // GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP, ...
		Vec3i resolution;
		uint32 mipmapLevels; // number of levels that the texture will have
		uint32 containedLevels; // number of levels that the asset contains
		uint32 channels;
		uint32 internalFormat;
		uint32 copyFormat;
		uint32 copyType;
		uint32 filterMin;
		uint32 filterMag;
		uint32 filterAniso;
		uint32 wrapX;
		uint32 wrapY;
		uint32 wrapZ;
		TextureSwizzleEnum swizzle[4];

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
		uint32 textureNames[MaxTexturesCountPerMaterial];
		uint32 shaderName;
		MeshRenderFlags renderFlags;
		sint32 renderLayer;
		uint32 skeletonBones;
		uint32 materialSize; // bytes

		// follows:
		// material (may or may not be the MeshImportMaterial)
		// serialized mesh
	};

	struct CAGE_ENGINE_API RenderObjectHeader
	{
		Vec3 color;
		Real intensity;
		Real opacity;

		Real texAnimSpeed;
		Real texAnimOffset;
		uint32 skelAnimName;
		Real skelAnimSpeed;
		Real skelAnimOffset;

		Real worldSize;
		Real pixelsSize;
		uint32 lodsCount;
		uint32 modelsCount;

		// follows:
		// array of thresholds, each float
		// array of model indices, each uint32 (one item more than thresholds)
		// array of model names, each uint32
	};

	struct CAGE_ENGINE_API FontHeader
	{
		uint32 ftSize; // bytes of font file
		uint32 imagesCount;
		uint32 glyphsCount;
		uint32 nominalSize; // used in FT_Set_Pixel_Sizes
		Real nominalScale; // used to convert font units to 1pt
		Real lineOffset; // pixels for 1pt
		Real lineHeight; // pixels for 1pt

		struct CAGE_ENGINE_API GlyphData
		{
			Vec4 texUv;
			uint32 imageIndex;
			uint32 glyphId;
		};

		// follows:
		// font file
		// array of images
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
		uint64 frames;
		uint32 channels;
		uint32 sampleRate;
		SoundCompressionEnum soundType;

		// follows (for raw file):
		// array of frames, each channels * float
		// ...

		// follows (for compressed file):
		// ogg/vorbis compressed stream
	};
}

#endif // guard_assets_h_5aade310_996b_42c7_8684_2100f6625d36_
