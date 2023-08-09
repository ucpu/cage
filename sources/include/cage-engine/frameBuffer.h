#ifndef guard_frameBuffer_h_mnbv1cw98e7rft
#define guard_frameBuffer_h_mnbv1cw98e7rft

#include <cage-engine/core.h>

namespace cage
{
	class Texture;

	class CAGE_ENGINE_API FrameBuffer : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		uint32 id() const;
		uint32 target() const;
		void bind() const;

		void depthTexture(Texture *tex);
		void colorTexture(uint32 index, Texture *tex, uint32 mipmapLevel = 0);
		void activeAttachments(uint32 mask);
		void clear();
		void checkStatus() const;
	};

	CAGE_ENGINE_API Holder<FrameBuffer> newFrameBufferDraw();
	CAGE_ENGINE_API Holder<FrameBuffer> newFrameBufferRead();
}

#endif // guard_frameBuffer_h_mnbv1cw98e7rft
