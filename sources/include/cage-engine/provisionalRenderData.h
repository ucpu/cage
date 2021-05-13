#ifndef guard_provisionalRenderData_h_dftrh5serft4l4u696f
#define guard_provisionalRenderData_h_dftrh5serft4l4u696f

#include "core.h"

namespace cage
{
	class CAGE_ENGINE_API ProvisionalTextureHandle : private Immovable
	{
	public:
		Holder<Texture> resolve(); // requires opengl context
	};

	class CAGE_ENGINE_API ProvisionalFrameBufferHandle : private Immovable
	{
	public:
		Holder<FrameBuffer> resolve(); // requires opengl context
	};

	class CAGE_ENGINE_API ProvisionalRenderData : private Immovable
	{
	public:
		// section: thread-safe, does not require opengl context
		
		Holder<ProvisionalTextureHandle> texture(const string &name);
		Holder<ProvisionalTextureHandle> texture(const string &name, uint32 target);
		Holder<ProvisionalTextureHandle> texture2dArray(const string &name);
		Holder<ProvisionalTextureHandle> textureRectangle(const string &name);
		Holder<ProvisionalTextureHandle> texture3d(const string &name);
		Holder<ProvisionalTextureHandle> textureCube(const string &name);

		Holder<ProvisionalFrameBufferHandle> frameBufferDraw(const string &name);
		Holder<ProvisionalFrameBufferHandle> frameBufferRead(const string &name);

		// section: thread-safe, requires opengl context

		void reset(); // erase data not used (resolved) since last reset
		void purge(); // erase all data unconditionally
	};

	CAGE_ENGINE_API Holder<ProvisionalRenderData> newProvisionalRenderData();
}

#endif // guard_provisionalRenderData_h_dftrh5serft4l4u696f
