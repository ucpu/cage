#ifndef guard_provisionalGraphics_h_dftrh5serft4l4u696f
#define guard_provisionalGraphics_h_dftrh5serft4l4u696f

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API ProvisionalUniformBuffer : private Immovable
	{
	public:
		Holder<UniformBuffer> resolve(); // requires opengl context
		bool ready() const;
	};

	class CAGE_ENGINE_API ProvisionalFrameBuffer : private Immovable
	{
	public:
		Holder<FrameBuffer> resolve(); // requires opengl context
		bool ready() const;
	};

	class CAGE_ENGINE_API ProvisionalTexture : private Immovable
	{
	public:
		Holder<Texture> resolve(); // requires opengl context
		bool ready() const;
	};

	class CAGE_ENGINE_API ProvisionalGraphics : private Immovable
	{
	public:
		// section: thread-safe, does not require opengl context

		Holder<ProvisionalUniformBuffer> uniformBuffer(const string &name);

		Holder<ProvisionalFrameBuffer> frameBufferDraw(const string &name);
		Holder<ProvisionalFrameBuffer> frameBufferRead(const string &name);

		Holder<ProvisionalTexture> texture(const string &name);
		Holder<ProvisionalTexture> texture(const string &name, uint32 target);
		Holder<ProvisionalTexture> texture2dArray(const string &name);
		Holder<ProvisionalTexture> textureRectangle(const string &name);
		Holder<ProvisionalTexture> texture3d(const string &name);
		Holder<ProvisionalTexture> textureCube(const string &name);

		// section: thread-safe, requires opengl context

		void reset(); // erase data not used (resolved) since last reset
		void purge(); // erase all data unconditionally
	};

	CAGE_ENGINE_API Holder<ProvisionalGraphics> newProvisionalGraphics();
}

#endif // guard_provisionalGraphics_h_dftrh5serft4l4u696f
