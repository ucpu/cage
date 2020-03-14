#include <cage-core/image.h>
#include <cage-core/math.h>

namespace cage
{
	namespace
	{
	}

	namespace detail
	{
		void imageResize(
			const float *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth,
			      float *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth,
			uint32 channels)
		{
			CAGE_THROW_CRITICAL(NotImplemented, "imageResize");
		}
	}
}
