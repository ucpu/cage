#ifndef guard_renderQueue_h_edrtz54sedr9wse4g4jk7
#define guard_renderQueue_h_edrtz54sedr9wse4g4jk7

#include "provisionalHandles.h"

namespace cage
{
	struct UubRange
	{
		uint32 offset = 0;
		uint32 size = 0;
	};

	class CAGE_ENGINE_API RenderQueue : private Immovable
	{
	public:
		// uses internal uniform buffer
		// its contents are sent to gpu memory all at once and individual ranges are bound as requested
		UubRange universalUniformBuffer(PointerRange<const char> data, uint32 bindingPoint = m);
		template<class T>
		UubRange universalUniformStruct(const T &data, uint32 bindingPoint = m)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(std::is_trivially_destructible_v<T>);
			static_assert(!std::is_pointer_v<T>);
			return universalUniformBuffer({ (const char *)&data, (const char *)(&data + 1) }, bindingPoint);
		}
		template<class T>
		UubRange universalUniformArray(PointerRange<const T> data, uint32 bindingPoint = m)
		{
			static_assert(std::is_trivially_copyable_v<T>);
			static_assert(std::is_trivially_destructible_v<T>);
			static_assert(!std::is_pointer_v<T>);
			return universalUniformBuffer({ (const char *)data.data(), (const char *)(data.data() + data.size()) }, bindingPoint);
		}
		void bind(UubRange uubRange, uint32 bindingPoint);

		void bind(UniformBufferHandle uniformBuffer, uint32 bindingPoint); // bind for reading from (on gpu)
		void bind(UniformBufferHandle uniformBuffer, uint32 bindingPoint, uint32 offset, uint32 size); // bind for reading from (on gpu)
		void bind(UniformBufferHandle uniformBuffer); // bind for writing into (on cpu)
		void writeWhole(PointerRange<const char> data, uint32 usage = 0);
		void writeRange(PointerRange<const char> data, uint32 offset = 0);

		void bind(const Holder<ShaderProgram> &shader);
#define GCHL_GENERATE(TYPE) \
		void uniform(uint32 name, const TYPE &value); \
		void uniform(uint32 name, PointerRange<const TYPE> values);
		GCHL_GENERATE(sint32);
		GCHL_GENERATE(uint32);
		GCHL_GENERATE(Vec2i);
		GCHL_GENERATE(Vec3i);
		GCHL_GENERATE(Vec4i);
		GCHL_GENERATE(Real);
		GCHL_GENERATE(Vec2);
		GCHL_GENERATE(Vec3);
		GCHL_GENERATE(Vec4);
		GCHL_GENERATE(Quat);
		GCHL_GENERATE(Mat3);
		GCHL_GENERATE(Mat4);
#undef GCHL_GENERATE

		void bind(FrameBufferHandle frameBuffer);
		void depthTexture(TextureHandle texture); // attach depth texture to current frame buffer
		void colorTexture(uint32 index, TextureHandle texture, uint32 mipmapLevel = 0); // attach color texture to current frame buffer
		void activeAttachments(uint32 mask); // bitmask of active color textures in current frame buffer
		void clearFrameBuffer(); // detach all textures from the frame buffer
		void checkFrameBuffer(); // check the frame buffer for completeness
		void resetFrameBuffer(); // bind default (0) frame buffer

		void bind(TextureHandle texture, uint32 bindingPoint);
		void image2d(Vec2i resolution, uint32 internalFormat);
		void imageCube(Vec2i resolution, uint32 internalFormat);
		void image3d(Vec3i resolution, uint32 internalFormat);
		void filters(uint32 mig, uint32 mag, uint32 aniso);
		void wraps(uint32 s, uint32 t);
		void wraps(uint32 s, uint32 t, uint32 r);
		void generateMipmaps();
		void resetAllTextures(); // bind default (0) texture in all texture units in all types

		void bind(const Holder<Model> &model);
		void draw(uint32 instances = 1);

		void viewport(Vec2i origin, Vec2i size);
		void scissors(Vec2i origin, Vec2i size);
		void scissors(bool enable);
		void cullFace(bool front);
		void culling(bool enable);
		void depthFunc(uint32 func);
		void depthFuncLess();
		void depthFuncLessEqual();
		void depthFuncAlways();
		void depthTest(bool enable);
		void depthWrite(bool enable);
		void colorWrite(bool enable);
		void blendFunc(uint32 s, uint32 d);
		void blendFuncNone(); // ONE, ZERO
		void blendFuncAdditive(); // ONE, ONE
		void blendFuncPremultipliedTransparency(); // ONE, ONE_MINUS_SRC_ALPHA
		void blendFuncAlphaTransparency(); // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
		void blending(bool enable);
		void clearColor(const Vec4 &rgba);
		void clear(bool color, bool depth, bool stencil = false);
		void genericEnable(uint32 key, bool enable);
		void resetAllState(); // set viewport, scissors, culling, depth, blend etc all to default values

		[[nodiscard]] struct RenderQueueNamedScope namedScope(StringLiteral name);

		// dispatch another queue as part of this queue
		// stores a reference to the queue - do not modify it after it has been enqueued
		void enqueue(const Holder<RenderQueue> &queue);

		void customCommand(Delegate<void(void *)> fnc, const Holder<void> &data, bool preservesGlState = false);

#ifdef CAGE_DEBUG
		void checkGlErrorDebug();
#else
		void checkGlErrorDebug() {}
#endif
		void checkGlError();

		void resetQueue(); // erase all stored commands

		void dispatch(ProvisionalGraphics *provisionalGraphics = nullptr); // requires opengl context bound in current thread

		uint32 commandsCount() const;
		uint32 drawsCount() const;
		uint32 primitivesCount() const;
	};

	CAGE_ENGINE_API Holder<RenderQueue> newRenderQueue();

	struct CAGE_ENGINE_API RenderQueueNamedScope : private Immovable
	{
		[[nodiscard]] RenderQueueNamedScope(RenderQueue *queue, StringLiteral name);
		~RenderQueueNamedScope();

	private:
		RenderQueue *queue = nullptr;
	};
}

#endif // guard_renderQueue_h_edrtz54sedr9wse4g4jk7
