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
		uint32 textureNames[MaxTexturesCountPerMaterial];
		uint32 materialSize;

		bool uvs() const;
		bool normals() const;
		bool tangents() const;
		bool bones() const;
		uint32 vertexSize() const;

		struct materialDataStruct
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
		uint32 namesCount;
		uint32 animationsCount;
	};

	struct CAGE_API animationHeaderStruct
	{
		uint64 duration; // in microseconds
		uint32 bonesCount;
		uint32 size; // in bytes
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
		uint32 glyphCount;
		uint32 charCount;
		uint32 texSize;
		uint16 texWidth, texHeight;
		uint16 lineHeight;
		uint16 glyphMaxWidth, glyphMaxHeight;
		sint16 glyphMaxBearingX, glyphMaxBearingY;

		struct glyphDataStruct
		{
			uint16 advance;
			uint16 width, height;
			uint16 texX, texY;
			sint16 bearX, bearY;
		};

		// follows:
		// texture data
		// array of glyphDataStruct
		// array of kerning, each sint8
		// array of characters, each uint32
		// array of glyphs, each uint32
	};

	struct CAGE_API soundHeaderStruct
	{
		soundTypeEnum soundType;
		soundFlags flags;
		uint32 frames;
		uint32 channels;
		uint32 sampleRate;

		// follows (for raw file):
		// <strike> array of samples, each 1 float, for first channel
		// array of samples, each 1 float, for second channel </strike>
		// array of frames, each channels count of floats
		// ...

		// follows (for compressed file):
		// ogg/vorbis compressed stream
	};
}
