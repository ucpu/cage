#ifndef guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_
#define guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_

#include "core.h"
#include <cage-core/geometry.h>

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
		uint32 dimX;
		uint32 dimY;
		uint32 dimZ;
		uint32 bpp; // bytes per pixel
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

	enum class MeshDataFlags : uint32
	{
		None = 0,
		Normals = 1 << 0,
		Tangents = 1 << 1,
		Bones = 1 << 2,
		Uvs = 1 << 3,
		Aux0 = 1 << 4,
		Aux1 = 1 << 5,
		Aux2 = 1 << 6,
		Aux3 = 1 << 7,
	};

	struct CAGE_ENGINE_API MeshHeader
	{
		aabb box;
		MeshDataFlags flags;
		uint32 primitiveType; // one of GL_POINTS, GL_LINES, GL_LINE_LOOP, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, ...
		uint32 verticesCount;
		uint32 indicesCount; // zero for non-indexed draws
		uint32 skeletonName;
		uint32 skeletonBones;
		uint32 textureNames[MaxTexturesCountPerMaterial];
		uint32 uvDimension;
		uint8 auxDimensions[4];
		uint32 instancesLimitHint;
		MeshRenderFlags renderFlags;
		uint32 materialSize;

		bool normals() const;
		bool tangents() const;
		bool bones() const;
		bool uvs() const;
		uint32 vertexSize() const;

		struct CAGE_ENGINE_API MaterialData
		{
			vec4 albedoBase;
			vec4 specialBase;
			vec4 albedoMult;
			vec4 specialMult;
			MaterialData();
		};

		// follows:
		// array of coordinates, each vec3
		// array of normals, each vec3, if meshFlags::Normals
		// array of tangents, each vec3, if meshFlags::Tangents
		// array of bitangents, each vec3, if meshFlags::Tangents
		// array of bone indices, each 4 * uint16, if meshFlags::Bones
		// array of bone weights, each 4 * float, if meshFlags::Bones
		// array of uvs, each vec* (the dimensionality is given in uvDimension), if MeshDataFlags::Uvs
		// array of auxiliary data, each vec*, if MeshDataFlags::aux0
		// array of auxiliary data, each vec*, if MeshDataFlags::aux1
		// array of auxiliary data, each vec*, if MeshDataFlags::aux2
		// array of auxiliary data, each vec*, if MeshDataFlags::aux3
		// array of indices, each uint32
		// material (may or may not be the MaterialData)

		// notes:
		// the four bone weights for each vertex must add to one
	};

	struct CAGE_ENGINE_API SkeletonRigHeader
	{
		mat4 globalInverse;
		uint32 bonesCount;

		// follows:
		// array of parent bone indices, each uint16, (uint16)-1 for the root
		// array of base transformation matrices, each mat4
		// array of inverted rest matrices, each mat4
	};

	struct CAGE_ENGINE_API SkeletalAnimationHeader
	{
		uint64 duration; // microseconds
		uint32 skeletonBonesCount;
		uint32 animationBonesCount;

		// follows:
		// array of indices, each uint16
		// array of position frames counts, each uint16
		// array of rotation frames counts, each uint16
		// array of scale frames counts, each uint16
		// array of bones, each:
		//   array of position times, each float
		//   array of position values, each vec3
		//   array of rotation times, each float
		//   array of rotation values, each quat
		//   array of scale times, each float
		//   array of scale values, each vec3
	};

	struct CAGE_ENGINE_API RenderObjectHeader
	{
		vec3 color;
		real opacity;
		
		real texAnimSpeed;
		real texAnimOffset;
		uint32 skelAnimName;
		real skelAnimSpeed;
		real skelAnimOffset;

		real worldSize;
		real pixelsSize;
		uint32 lodsCount;
		uint32 meshesCount;

		// follows:
		// array of thresholds, each float
		// array of mesh indices, each uint32 (one item more than thresholds)
		// array of mesh names, each uint32
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
		uint32 texWidth, texHeight; // texels
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
		SoundTypeEnum soundType;
		SoundFlags flags;
		uint32 frames;
		uint32 channels;
		uint32 sampleRate;

		// follows (for raw file):
		// array of frames, each channels * float
		// ...

		// follows (for compressed file):
		// ogg/vorbis compressed stream
	};
}

#endif // guard_assets_h_5aade310_996b_42c7_8684_2100f6625d36_
