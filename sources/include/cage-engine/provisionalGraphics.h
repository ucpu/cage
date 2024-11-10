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

		// section: thread-safe, requires opengl context

		void reset(); // erase data not used (resolved) since last reset
		void purge(); // erase all data unconditionally
	};

	CAGE_ENGINE_API Holder<ProvisionalGraphics> newProvisionalGraphics();
}

#endif // guard_provisionalGraphics_h_dftrh5serft4l4u696f
