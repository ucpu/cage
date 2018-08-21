#ifndef guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_
#define guard_asset_structs_h_5aade310_996b_42c7_8684_2100f6625d36_

namespace cage
{
	/*
	struct CAGE_API shaderHeaderStruct
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

	struct CAGE_API textureHeaderStruct
	{
		textureFlags flags;
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
		uint32 animationDuration;

		// follows:
		// array of array of texels
	};

	struct CAGE_API meshHeaderStruct
	{
		aabb box;
		meshFlags flags;
		uint32 primitiveType; // one of GL_POINTS, GL_LINES, GL_LINE_LOOP, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, ...
		uint32 verticesCount;
		uint32 indicesCount; // zero for non-indexed draws
		uint32 skeletonName;
		uint32 skeletonBones;
		uint32 textureNames[MaxTexturesCountPerMaterial];
		uint32 materialSize;

		bool uvs() const;
		bool normals() const;
		bool tangents() const;
		bool bones() const;
		uint32 vertexSize() const;

		struct CAGE_API materialDataStruct
		{
			vec4 albedoBase;
			vec4 specialBase;
			vec4 albedoMult;
			vec4 specialMult;
		};

		// follows:
		// material (may or may not be the materialDataStruct)
		// array of coordinates, each vec3
		// array of uv, each vec2, if meshFlags::Uvs
		// array of normals, each vec3, if meshFlags::Normals
		// array of tangents, each vec3, if meshFlags::Tangents
		// array of bitangents, each vec3, if meshFlags::Tangents
		// array of bone indices, each 4 * uint16, if meshFlags::Bones
		// array of bone weight, each 4 * float, if meshFlags::Bones
		// array of indices, each uint32

		// notes:
		// the four bone weights for each vertex must add to one
		// meshFlags::Transparency and meshFlags::Translucency are mutually exclusive
	};

	struct CAGE_API skeletonHeaderStruct
	{
		uint32 bonesCount;

		// follows:
		// array of parent bone indices, each uint16, (uint16)-1 for the root
		// array of base transformation matrices, each mat4
		// array of inverted rest matrices, each mat4
	};

	struct CAGE_API animationHeaderStruct
	{
		uint64 duration; // in microseconds
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

	struct CAGE_API objectHeaderStruct
	{
		uint32 shadower;
		uint32 collider;
		real worldSize;
		real pixelsSize;
		uint32 lodsCount;

		// follows:
		// threshold, float, for first lod
		// count, uint32, number of meshes in this lod level
		// array of meshes, each uint32
		// threshold, float, for second lod
		// ...
	};

	struct CAGE_API fontHeaderStruct
	{
		fontFlags flags;
		vec2 glyphMaxSize; // linear units
		real lineHeight; // linear units
		real firstLineOffset; // linear units
		uint32 texSize; // bytes
		uint32 texWidth, texHeight; // texels
		uint32 glyphCount;
		uint32 charCount;

		struct CAGE_API glyphDataStruct
		{
			vec4 texUv;
			vec2 size; // linear units
			vec2 bearing; // linear units
			real advance; // linear units
		};

		// follows:
		// texture data
		// array of glyphDataStruct
		// array of kerning, each 1 real
		// array of charset characters, each uint32
		// array of charset glyphs, each uint32

		// notes:
		// linear units are pixels for 1pt-font
		// linear units must be multiplied by font size before rendering
	};

	struct CAGE_API soundHeaderStruct
	{
		soundTypeEnum soundType;
		soundFlags flags;
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
