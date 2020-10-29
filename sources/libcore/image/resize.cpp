#include <cage-core/image.h>
#include <cage-core/memoryUtils.h>

#include <stb_image_resize.h>

namespace cage
{
	namespace detail
	{
		void imageResize(
			const float *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth,
			      float *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth,
			uint32 channels)
		{
			if (sourceDepth != 1 || targetDepth != 1)
			{
				CAGE_THROW_CRITICAL(NotImplemented, "3D image resize");
			}

			auto res = stbir_resize_float(sourceData, sourceWidth, sourceHeight, 0,
				targetData, targetWidth, targetHeight, 0, channels);

			if (res != 1)
			{
				CAGE_THROW_ERROR(OutOfMemory, "image resize", 0);
			}
		}
	}
}
