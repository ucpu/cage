#ifndef guard_renderQueue_h_edrtz54sedr9wse4g4jk7
#define guard_renderQueue_h_edrtz54sedr9wse4g4jk7

#include "core.h"

namespace cage
{
	struct RenderQueueNamedPassScope;

	class CAGE_ENGINE_API RenderQueue : private Immovable
	{
	public:
		// uses internal uniform buffer that is populated at start of dispatch of this queue
		// the contents are sent to gpu memory all at once and individual ranges are bound as requested
		void universalUniformBuffer(uint32 bindingPoint, PointerRange<const char> data);

		template<class T>
		void universalUniformStruct(uint32 bindingPoint, const T &data)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(std::is_trivially_destructible_v<T>);
			static_assert(!std::is_pointer_v<T>);
			universalUniformBuffer(bindingPoint, { (const char *)&data, (const char *)(&data + 1) });
		}

		template<class T>
		void universalUniformArray(uint32 bindingPoint, PointerRange<const T> data)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(std::is_trivially_destructible_v<T>);
			static_assert(!std::is_pointer_v<T>);
			universalUniformBuffer(bindingPoint, { (const char *)data.data(), (const char *)(data.data() + data.size()) });
		}

		void bind(Holder<UniformBuffer> &&uniformBuffer, uint32 bindingPoint); // bind for reading from (on gpu)
		void bind(Holder<UniformBuffer> &&uniformBuffer, uint32 bindingPoint, uint32 offset, uint32 size); // bind for reading from (on gpu)
		void bind(Holder<UniformBuffer> &&uniformBuffer); // bind for writing into (on cpu)
		void writeWhole(PointerRange<const char> data, uint32 usage = 0);
		void writeRange(PointerRange<const char> data, uint32 offset = 0);

		void bind(Holder<ShaderProgram> &&shader);
#define GCHL_GENERATE(TYPE) \
		void uniform(uint32 name, const TYPE &value); \
		void uniform(uint32 name, PointerRange<const TYPE> values);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(ivec2);
		GCHL_GENERATE(ivec3);
		GCHL_GENERATE(ivec4);
		GCHL_GENERATE(real);
		GCHL_GENERATE(vec2);
		GCHL_GENERATE(vec3);
		GCHL_GENERATE(vec4);
		GCHL_GENERATE(quat);
		GCHL_GENERATE(mat3);
		GCHL_GENERATE(mat4);
#undef GCHL_GENERATE

		void bind(Holder<FrameBuffer> &&framebuffer);
		void depthTexture(Holder<Texture> &&texture);
		void colorTexture(uint32 index, Holder<Texture> &&texture, uint32 mipmapLevel = 0);
		void activeAttachments(uint32 mask);
		void clearFrameBuffer();

		void bind(Holder<Texture> &&texture, uint32 bindingPoint);
		void image2d(uint32 w, uint32 h, uint32 internalFormat);
		void imageCube(uint32 w, uint32 h, uint32 internalFormat);
		void image3d(uint32 w, uint32 h, uint32 d, uint32 internalFormat);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void generateMipmaps();
		void unbindAllTextures();

		void bind(Holder<Model> &&model);
		void draw(uint32 instances = 1);

		void viewport(ivec2 origin, ivec2 size);
		void scissors(ivec2 origin, ivec2 size);
		void scissors(bool enable);
		void cullingFace(bool front);
		void culling(bool enable);
		void depthFunc(uint32 func);
		void depthFuncAlways();
		void depthFuncLessEqual();
		void depthTest(bool enable);
		void depthWrite(bool enable);
		void colorWrite(bool enable);
		void blendFunc(uint32 s, uint32 d);
		void blendFuncAdditive(); // ONE, ONE
		void blendFuncPremultipliedTransparency(); // ONE, ONE_MINUS_SRC_ALPHA
		void blendFuncAlphaTransparency(); // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
		void blending(bool enable);
		void clearColor(const vec4 &rgba);
		void clear(bool color, bool depth, bool stencil = false);
		void genericEnable(uint32 key, bool enable);

		void pushNamedPass(const string &name);
		void popNamedPass();
		[[nodiscard]] RenderQueueNamedPassScope scopedNamedPass(const string &name);

		void reset(); // erase all stored commands

		void dispatch(); // requires opengl context bound in current thread

		uint32 commandsCount() const;
		uint32 drawsCount() const;
		uint32 primitivesCount() const;
	};

	struct RenderQueueNamedPassScope : Immovable
	{
		RenderQueue *queue = nullptr;
		[[nodiscard]] RenderQueueNamedPassScope(RenderQueue *queue, const string &name);
		~RenderQueueNamedPassScope();
	};

	CAGE_ENGINE_API Holder<RenderQueue> newRenderQueue();
}

#endif // guard_renderQueue_h_edrtz54sedr9wse4g4jk7
