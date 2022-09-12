#ifndef guard_provisionalGraphics_h_dftrh5serft4l4u696f
#define guard_provisionalGraphics_h_dftrh5serft4l4u696f

#include "core.h"

namespace cage
{
	class UniformBuffer;
	class FrameBuffer;
	class Texture;

	class CAGE_ENGINE_API ProvisionalUniformBuffer : private Immovable
	{
	public:
		Holder<UniformBuffer> resolve(); // requires opengl context
		bool ready() const;
		bool first(); // return true the first time it is called on this provisional buffer
	};

	class CAGE_ENGINE_API ProvisionalFrameBuffer : private Immovable
	{
	public:
		Holder<FrameBuffer> resolve(); // requires opengl context
		bool ready() const;
		bool first(); // return true the first time it is called on this provisional buffer
	};

	class CAGE_ENGINE_API ProvisionalTexture : private Immovable
	{
	public:
		Holder<Texture> resolve(); // requires opengl context
		bool ready() const;
		bool first(); // return true the first time it is called on this provisional texture
	};

	class CAGE_ENGINE_API ProvisionalGraphics : private Immovable
	{
	public:
		// section: thread-safe, does not require opengl context

		Holder<ProvisionalUniformBuffer> uniformBuffer(const String &name);

		Holder<ProvisionalFrameBuffer> frameBufferDraw(const String &name);
		Holder<ProvisionalFrameBuffer> frameBufferRead(const String &name);

		Holder<ProvisionalTexture> texture(const String &name);
		Holder<ProvisionalTexture> texture(const String &name, uint32 target);
		Holder<ProvisionalTexture> texture2dArray(const String &name);
		Holder<ProvisionalTexture> textureRectangle(const String &name);
		Holder<ProvisionalTexture> texture3d(const String &name);
		Holder<ProvisionalTexture> textureCube(const String &name);

		// section: thread-safe, requires opengl context

		void reset(); // erase data not used (resolved) since last reset
		void purge(); // erase all data unconditionally
	};

	CAGE_ENGINE_API Holder<ProvisionalGraphics> newProvisionalGraphics();
}

#endif // guard_provisionalGraphics_h_dftrh5serft4l4u696f
