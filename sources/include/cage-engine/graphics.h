#ifndef guard_graphics_h_d776d3a2_43c7_464d_b721_291294b5b1ef_
#define guard_graphics_h_d776d3a2_43c7_464d_b721_291294b5b1ef_

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
		using SystemError::SystemError;
	};

	struct CAGE_ENGINE_API GraphicsDebugScope : private Immovable
	{
		GraphicsDebugScope(const char *name);
		~GraphicsDebugScope();
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

		uint64 animationDuration = 0;
		bool animationLoop = false;

		uint32 getId() const;
		uint32 getTarget() const;
		void getResolution(uint32 &width, uint32 &height) const;
		void getResolution(uint32 &width, uint32 &height, uint32 &depth) const;
		void bind() const;

		void importImage(const Image *img);
		void image2d(uint32 w, uint32 h, uint32 internalFormat);
		void image2d(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer, uintPtr stride = 0);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat, uint32 format, uint32 type, PointerRange<const char> buffer);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void generateMipmaps();

		[[deprecated]] static void multiBind(uint32 count, const uint32 tius[], const Texture *const texs[]);
		static void multiBind(PointerRange<const uint32> tius, PointerRange<const Texture *const> texs);
	};

	CAGE_ENGINE_API Holder<Texture> newTexture();
	CAGE_ENGINE_API Holder<Texture> newTexture(uint32 target);

	namespace detail
	{
		CAGE_ENGINE_API vec4 evalSamplesForTextureAnimation(const Texture *texture, uint64 emitTime, uint64 animationStart, real animationSpeed, real animationOffset);
	}

	CAGE_ENGINE_API AssetScheme genAssetSchemeTexture(uint32 threadIndex);
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

		void source(uint32 type, PointerRange<const char> buffer);
		void relink();
		void validate();

#define GCHL_GENERATE(TYPE) \
		void uniform(uint32 name, const TYPE &value); \
		void uniform(uint32 name, PointerRange<const TYPE> values);
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

	CAGE_ENGINE_API AssetScheme genAssetSchemeShaderProgram(uint32 threadIndex);
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

		void writeWhole(PointerRange<const char> buffer, uint32 usage = 0);
		void writeRange(PointerRange<const char> buffer, uint32 offset);

		uint32 getSize() const;

		static uint32 getAlignmentRequirement();
	};

	struct CAGE_ENGINE_API UniformBufferCreateConfig
	{
		uint32 size = 0;
		bool persistentMapped = false, coherentMapped = false, explicitFlush = false;
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
		void setImage(uint32 width, uint32 height, PointerRange<const char> buffer);
		void setGlyphs(PointerRange<const char> buffer, PointerRange<const real> kerning);
		void setCharmap(PointerRange<const uint32> chars, PointerRange<const uint32> glyphs);

		struct CAGE_ENGINE_API FormatStruct
		{
			real size = 13;
			real wrapWidth = real::Infinity();
			real lineSpacing = 0;
			TextAlignEnum align = TextAlignEnum::Left;
		};

		uint32 glyphsCount(const string &text);
		uint32 glyphsCount(const char *text);
		uint32 glyphsCount(PointerRange<const char> text);

		void transcript(const string &text, PointerRange<uint32> glyphs);
		void transcript(const char *text, PointerRange<uint32> glyphs);
		void transcript(PointerRange<const char> text, PointerRange<uint32> glyphs);

		vec2 size(PointerRange<const uint32> glyphs, const FormatStruct &format);
		vec2 size(PointerRange<const uint32> glyphs, const FormatStruct &format, const vec2 &mousePosition, uint32 &cursor);

		void bind(Model *model, ShaderProgram *shader) const;
		void render(PointerRange<const uint32> glyphs, const FormatStruct &format, uint32 cursor = m);
	};

	CAGE_ENGINE_API Holder<Font> newFont();

	CAGE_ENGINE_API AssetScheme genAssetSchemeFont(uint32 threadIndex);
	static constexpr uint32 AssetSchemeIndexFont = 14;

	class CAGE_ENGINE_API Model : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const string &name);

		uint32 getId() const;
		void bind() const;

		void importMesh(const Mesh *poly, PointerRange<const char> materialBuffer);

		void setFlags(ModelRenderFlags flags);
		void setPrimitiveType(uint32 type);
		void setBoundingBox(const Aabb &box);
		void setTextureNames(PointerRange<const uint32> textureNames);
		void setTextureName(uint32 index, uint32 name);
		void setBuffers(uint32 vertexSize, PointerRange<const char> vertexData, PointerRange<const uint32> indexData, PointerRange<const char> materialBuffer);
		void setAttribute(uint32 index, uint32 size, uint32 type, uint32 stride, uint32 startOffset);
		void setSkeleton(uint32 name, uint32 bones);
		void setInstancesLimitHint(uint32 limit);

		uint32 getVerticesCount() const;
		uint32 getIndicesCount() const;
		uint32 getPrimitivesCount() const;
		ModelRenderFlags getFlags() const;
		Aabb getBoundingBox() const;
		PointerRange<const uint32> getTextureNames() const;
		uint32 getTextureName(uint32 index) const;
		uint32 getSkeletonName() const;
		uint32 getSkeletonBones() const;
		uint32 getInstancesLimitHint() const;

		void dispatch() const;
		void dispatch(uint32 instances) const;
	};

	CAGE_ENGINE_API Holder<Model> newModel();

	CAGE_ENGINE_API AssetScheme genAssetSchemeModel(uint32 threadIndex);
	static constexpr uint32 AssetSchemeIndexModel = 12;

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
		void setLods(PointerRange<const real> thresholds, PointerRange<const uint32> modelIndices, PointerRange<const uint32> modelNames);
		uint32 lodsCount() const;
		uint32 lodSelect(real threshold) const;
		PointerRange<const uint32> models(uint32 lod) const;

		// default values for rendering

		vec3 color = vec3::Nan();
		real intensity = real::Nan();
		real opacity = real::Nan();
		real texAnimSpeed = real::Nan();
		real texAnimOffset = real::Nan();
		uint32 skelAnimName = 0;
		real skelAnimSpeed = real::Nan();
		real skelAnimOffset = real::Nan();
	};

	CAGE_ENGINE_API Holder<RenderObject> newRenderObject();

	CAGE_ENGINE_API AssetScheme genAssetSchemeRenderObject();
	static constexpr uint32 AssetSchemeIndexRenderObject = 13;
}

#endif // guard_graphics_h_d776d3a2_43c7_464d_b721_291294b5b1ef_
