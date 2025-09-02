#ifndef guard_provisionalGraphics_h_dftrh5serft4l4u696f
#define guard_provisionalGraphics_h_dftrh5serft4l4u696f

#include <functional>

#include <cage-engine/core.h>

namespace cage
{
	class UniformBuffer;
	class FrameBuffer;
	class Texture;

	class CAGE_ENGINE_API ProvisionalUniformBuffer : private Immovable
	{
	public:
		Holder<UniformBuffer> resolve(); // requires opengl context
	};

	class CAGE_ENGINE_API ProvisionalFrameBuffer : private Immovable
	{
	public:
		Holder<FrameBuffer> resolve(); // requires opengl context
	};

	class CAGE_ENGINE_API ProvisionalTexture : private Immovable
	{
	public:
		Holder<Texture> resolve(); // requires opengl context
	};

	class CAGE_ENGINE_API ProvisionalGraphics : private Immovable
	{
	public:
		// section: thread-safe, does not require opengl context

		Holder<ProvisionalUniformBuffer> uniformBuffer(const String &name, std::function<void(UniformBuffer *)> &&init = {});

		Holder<ProvisionalFrameBuffer> frameBufferDraw(const String &name);
		Holder<ProvisionalFrameBuffer> frameBufferRead(const String &name);

		Holder<ProvisionalTexture> texture(const String &name, std::function<void(Texture *)> &&init = {});
		Holder<ProvisionalTexture> texture(const String &name, uint32 target, std::function<void(Texture *)> &&init = {});

		template<class T>
		requires(std::is_default_constructible_v<T> && std::is_trivially_destructible_v<T> && std::is_same_v<std::remove_cvref_t<T>, T>)
		Holder<T> cpuBuffer(const String &name)
		{
			auto h = cpuBuffer_(name, sizeof(T), [](void *ptr) -> void { new (ptr, privat::OperatorNewTrait()) T(); });
			T *ptr = (T *)+h;
			return Holder<T>(ptr, std::move(h));
		}

		// section: thread-safe, requires opengl context

		void update(); // erase data not used (resolved) since last update
		void purge(); // erase all data unconditionally

	private:
		Holder<void> cpuBuffer_(const String &name, uintPtr size, Delegate<void(void *)> &&init);
	};

	CAGE_ENGINE_API Holder<ProvisionalGraphics> newProvisionalGraphics();
}

#endif // guard_provisionalGraphics_h_dftrh5serft4l4u696f
