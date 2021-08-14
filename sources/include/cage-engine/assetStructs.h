#ifndef guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_
#define guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_

#include <cage-core/geometry.h>

#include "core.h"

namespace cage
{
	/*
	struct CAGE_ENGINE_API ShaderProgramHeader
	{
		// follows:
		// number of stages, uint32
		// stage name, uint32
		// length, uint32
		// stage code, array of chars
		// stage name, uint32
		// ...
	};
	*/

	enum class TextureFlags : uint32
	{
		None = 0,
		GenerateMipmaps = 1 << 0,
		AnimationLoop = 1 << 1,
		AllowDownscale = 1 << 2,
	};

	struct CAGE_ENGINE_API TextureHeader
	{
		uint64 animationDuration;
		TextureFlags flags;
		uint32 target; // GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP, ...
		ivec3 resolution;
		uint32 channels;
		uint32 stride; // only used for GL_TEXTURE_CUBE_MAP, otherwise 0
		uint32 internalFormat;
		uint32 copyFormat;
		uint32 copyType;
		uint32 filterMin;
		uint32 filterMag;
		uint32 filterAniso;
		uint32 wrapX;
		uint32 wrapY;
		uint32 wrapZ;

		// follows:
		// array of texels
	};

	struct CAGE_ENGINE_API ModelHeader
	{
		Aabb box;
		uint32 textureNames[MaxTexturesCountPerMaterial];
		uint32 materialSize;
		uint32 skeletonBones;
		ModelRenderFlags renderFlags;

		struct CAGE_ENGINE_API MaterialData
		{
			// albedo rgb is linear, and NOT alpha-premultiplied
			vec4 albedoBase = vec4(0);
			vec4 specialBase = vec4(0);
			vec4 albedoMult = vec4(1);
			vec4 specialMult = vec4(1);
		};

		// follows:
		// material (may or may not be the MaterialData)
		// serialized mesh
	};

	struct CAGE_ENGINE_API RenderObjectHeader
	{
		vec3 color;
		real intensity;
		real opacity;
		
		real texAnimSpeed;
		real texAnimOffset;
		uint32 skelAnimName;
		real skelAnimSpeed;
		real skelAnimOffset;

		real worldSize;
		real pixelsSize;
		uint32 lodsCount;
		uint32 modelesCount;

		// follows:
		// array of thresholds, each float
		// array of model indices, each uint32 (one item more than thresholds)
		// array of model names, each uint32
	};

	enum class FontFlags : uint32
	{
		None = 0,
		Kerning = 1 << 0,
	};

	struct CAGE_ENGINE_API FontHeader
	{
		FontFlags flags;
		vec2 glyphMaxSize; // linear units
		real lineHeight; // linear units
		real firstLineOffset; // linear units
		uint32 texSize; // bytes
		ivec2 texResolution;
		uint32 glyphCount;
		uint32 charCount;

		struct CAGE_ENGINE_API GlyphData
		{
			vec4 texUv;
			vec2 size; // linear units
			vec2 bearing; // linear units
			real advance; // linear units
		};

		// follows:
		// texture data
		// array of GlyphData
		// array of kerning, each 1 real
		// array of charset characters, each uint32
		// array of charset glyphs, each uint32

		// notes:
		// linear units are pixels for 1pt-font
		// linear units must be multiplied by font size before rendering
	};

	enum class SoundTypeEnum : uint32
	{
		RawRaw, // short sounds
		CompressedRaw, // long sounds, but played many times concurrently
		CompressedCompressed, // long sounds, usually only played once at any given time
	};

	enum class SoundFlags : uint32
	{
		None = 0,
		LoopBeforeStart = 1 << 0,
		LoopAfterEnd = 1 << 1,
	};

	struct CAGE_ENGINE_API SoundSourceHeader
	{
		uint64 frames;
		uint32 channels;
		uint32 sampleRate;
		real referenceDistance;
		real rolloffFactor;
		real gain;
		SoundTypeEnum soundType;
		SoundFlags flags;

		// follows (for raw file):
		// array of frames, each channels * float
		// ...

		// follows (for compressed file):
		// ogg/vorbis compressed stream
	};
}

#endif // guard_assets_h_5aade310_996b_42c7_8684_2100f6625d36_
