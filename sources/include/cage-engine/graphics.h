#ifndef guard_graphic_h_d776d3a2_43c7_464d_b721_291294b5b1ef_
#define guard_graphic_h_d776d3a2_43c7_464d_b721_291294b5b1ef_

#include "core.h"

namespace cage
{
	CAGE_ENGINE_API void checkGlError();

#ifdef CAGE_DEBUG
#define CAGE_CHECK_GL_ERROR_DEBUG() { try { checkGlError(); } catch (const ::cage::GraphicsError &) { CAGE_LOG(::cage::SeverityEnum::Error, "exception", ::cage::stringizer() + "opengl error caught in file '" + __FILE__ + "' at line " + __LINE__); } }
#else
#define CAGE_CHECK_GL_ERROR_DEBUG() {}
#endif

	struct CAGE_ENGINE_API GraphicsError : public SystemError
	{
		GraphicsError(const char *file, uint32 line, const char *function, SeverityEnum severity, const char *message, uint32 code) noexcept;
	};

	namespace detail
	{
		CAGE_ENGINE_API void purgeGlShaderCache();
	}

	class CAGE_ENGINE_API Texture : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint64 animationDuration;
		bool animationLoop;

		uint32 getId() const;
		uint32 getTarget() const;
		void getResolution(uint32 &width, uint32 &height) const;
		void getResolution(uint32 &width, uint32 &height, uint32 &depth) const;
		void bind() const;

		void image2d(uint32 w, uint32 h, uint32 internalFormat);
		void image2d(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, const void *data);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, const void *data, uintPtr stride = 0);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat, uint32 format, uint32 type, const void *data);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void generateMipmaps();

		static void multiBind(uint32 count, const uint32 tius[], const Texture *const texs[]);
	};

	CAGE_ENGINE_API Holder<Texture> newTexture();
	CAGE_ENGINE_API Holder<Texture> newTexture(uint32 target);

	namespace detail
	{
		CAGE_ENGINE_API vec4 evalSamplesForTextureAnimation(const Texture *texture, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}

	CAGE_ENGINE_API AssetScheme genAssetSchemeTexture(uint32 threadIndex, Window *memoryContext);
	static constexpr uint32 AssetSchemeIndexTexture = 11;

	class CAGE_ENGINE_API FrameBuffer : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		uint32 getTarget() const;
		void bind() const;

		void depthTexture(Texture *tex);
		void colorTexture(uint32 index, Texture *tex, uint32 mipmapLevel = 0);
		void activeAttachments(uint32 mask);
		void clear();
		void checkStatus();
	};

	CAGE_ENGINE_API Holder<FrameBuffer> newFrameBufferDraw();
	CAGE_ENGINE_API Holder<FrameBuffer> newFrameBufferRead();

	class CAGE_ENGINE_API ShaderProgram : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;

		void source(uint32 type, const char *data, uint32 length);
		void relink();
		void validate();

#define GCHL_GENERATE(TYPE) \
		void uniform(uint32 name, const TYPE &value); \
		void uniform(uint32 name, const TYPE *values, uint32 count);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(real);
		GCHL_GENERATE(vec2);
		GCHL_GENERATE(vec3);
		GCHL_GENERATE(vec4);
		GCHL_GENERATE(quat);
		GCHL_GENERATE(mat3);
		GCHL_GENERATE(mat4);
#undef GCHL_GENERATE
	};

	CAGE_ENGINE_API Holder<ShaderProgram> newShaderProgram();

	CAGE_ENGINE_API AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex, Window *memoryContext);
	static constexpr uint32 AssetSchemeIndexShaderProgram = 10;

	class CAGE_ENGINE_API UniformBuffer : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;
		void bind(uint32 bindingPoint) const;
		void bind(uint32 bindingPoint, uint32 offset, uint32 size) const;

		void writeWhole(const void *data, uint32 size, uint32 usage = 0);
		void writeRange(const void *data, uint32 offset, uint32 size);

		uint32 getSize() const;

		static uint32 getAlignmentRequirement();
	};

	struct CAGE_ENGINE_API UniformBufferCreateConfig
	{
		uint32 size;
		bool persistentMapped, coherentMapped, explicitFlush;

		UniformBufferCreateConfig();
	};

	CAGE_ENGINE_API Holder<UniformBuffer> newUniformBuffer(const UniformBufferCreateConfig &config = {});

	class CAGE_ENGINE_API Font : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void setLine(real lineHeight, real firstLineOffset);
		void setImage(uint32 width, uint32 height, uint32 size, const void *data);
		void setGlyphs(uint32 count, const void *data, const real *kerning);
		void setCharmap(uint32 count, const uint32 *chars, const uint32 *glyphs);

		struct CAGE_ENGINE_API FormatStruct
		{
			real size;
			real wrapWidth;
			real lineSpacing;
			TextAlignEnum align;
			FormatStruct();
		};

		void transcript(const string &text, uint32 *glyphs, uint32 &count);
		void transcript(const char *text, uint32 *glyphs, uint32 &count);

		void size(const uint32 *glyphs, uint32 count, const FormatStruct &format, vec2 &size);
		void size(const uint32 *glyphs, uint32 count, const FormatStruct &format, vec2 &size, const vec2 &mousePosition, uint32 &cursor);

		void bind(Mesh *mesh, ShaderProgram *shader) const;
		void render(const uint32 *glyphs, uint32 count, const FormatStruct &format, uint32 cursor = m);
	};

	CAGE_ENGINE_API Holder<Font> newFont();

	CAGE_ENGINE_API AssetScheme genAssetSchemeFont(uint32 threadIndex, Window *memoryContext);
	static constexpr uint32 AssetSchemeIndexFont = 16;

	class CAGE_ENGINE_API Mesh : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;

		void setFlags(MeshRenderFlags flags);
		void setPrimitiveType(uint32 type);
		void setBoundingBox(const aabb &box);
		void setTextureNames(const uint32 *textureNames);
		void setTextureName(uint32 index, uint32 name);
		void setBuffers(uint32 verticesCount, uint32 vertexSize, const void *vertexData, uint32 indicesCount, const uint32 *indexData, uint32 materialSize, const void *MaterialData);
		void setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset);
		void setSkeleton(uint32 name, uint32 bones);
		void setInstancesLimitHint(uint32 limit);

		uint32 getVerticesCount() const;
		uint32 getIndicesCount() const;
		uint32 getPrimitivesCount() const;
		MeshRenderFlags getFlags() const;
		aabb getBoundingBox() const;
		const uint32 *getTextureNames() const;
		uint32 getTextureName(uint32 texIdx) const;
		uint32 getSkeletonName() const;
		uint32 getSkeletonBones() const;
		uint32 getInstancesLimitHint() const;

		void dispatch() const;
		void dispatch(uint32 instances) const;
	};

	CAGE_ENGINE_API Holder<Mesh> newMesh();

	CAGE_ENGINE_API AssetScheme genAssetSchemeMesh(uint32 threadIndex, Window *memoryContext);
	static constexpr uint32 AssetSchemeIndexMesh = 12;

	class CAGE_ENGINE_API SkeletalAnimation : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void allocate(uint64 duration, uint32 bones, const uint16 *indexes, const uint16 *positionFrames, const uint16 *rotationFrames, const uint16 *scaleFrames, const void *data);

		mat4 evaluate(uint16 bone, real coef) const;
		uint64 duration() const;
	};

	CAGE_ENGINE_API Holder<SkeletalAnimation> newSkeletalAnimation();

	namespace detail
	{
		CAGE_ENGINE_API real evalCoefficientForSkeletalAnimation(SkeletalAnimation *animation, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}

	CAGE_ENGINE_API AssetScheme genAssetSchemeSkeletalAnimation();
	static constexpr uint32 AssetSchemeIndexSkeletalAnimation = 14;

	class CAGE_ENGINE_API SkeletonRig : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		void allocate(const mat4 &globalInverse, uint32 totalBones, const uint16 *boneParents, const mat4 *baseMatrices, const mat4 *invRestMatrices);

		uint32 bonesCount() const;
		void animateSkin(const SkeletalAnimation *animation, real coef, mat4 *temporary, mat4 *output) const;
		void animateSkeleton(const SkeletalAnimation *animation, real coef, mat4 *temporary, mat4 *output) const;
	};

	CAGE_ENGINE_API Holder<SkeletonRig> newSkeletonRig();

	CAGE_ENGINE_API AssetScheme genAssetSchemeSkeletonRig();
	static constexpr uint32 AssetSchemeIndexSkeletonRig = 13;

	class CAGE_ENGINE_API RenderObject : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		// lod selection properties

		real worldSize;
		real pixelsSize;
		void setLods(uint32 lodsCount, uint32 meshesCount, const float *thresholds, const uint32 *meshIndices, const uint32 *meshNames);
		uint32 lodsCount() const;
		uint32 lodSelect(float threshold) const;
		PointerRange<const uint32> meshes(uint32 lod) const;

		// default values for rendering

		vec3 color;
		real intensity;
		real opacity;
		real texAnimSpeed;
		real texAnimOffset;
		uint32 skelAnimName;
		real skelAnimSpeed;
		real skelAnimOffset;

		RenderObject();
	};

	CAGE_ENGINE_API Holder<RenderObject> newRenderObject();

	CAGE_ENGINE_API AssetScheme genAssetSchemeRenderObject();
	static constexpr uint32 AssetSchemeIndexRenderObject = 15;
}

#endif // guard_graphic_h_d776d3a2_43c7_464d_b721_291294b5b1ef_
