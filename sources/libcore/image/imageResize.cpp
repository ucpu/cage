#include <cage-core/image.h>
#include <cage-core/memoryUtils.h>
#include <avir/lancir.h>

namespace cage
{
	namespace
	{
		template<class T>
		void imageResizeImpl(
			const T *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth,
			      T *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth,
			uint32 channels)
		{
			if (sourceDepth != 1 || targetDepth != 1)
			{
				CAGE_THROW_CRITICAL(NotImplemented, "3D image resize");
			}

			avir::CLancIR ir;

			ir.resizeImage<T, T>(sourceData, sourceWidth, sourceHeight, 0, targetData, targetWidth, targetHeight, 0, channels);
		}
	}

	namespace detail
	{
		void imageResize(
			const uint8 *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth,
			      uint8 *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth,
			uint32 channels)
		{
			imageResizeImpl<uint8>(
				sourceData, sourceWidth, sourceHeight, sourceDepth,
				targetData, targetWidth, targetHeight, targetDepth,
				channels);
		}

		void imageResize(
			const float *sourceData, uint32 sourceWidth, uint32 sourceHeight, uint32 sourceDepth,
			      float *targetData, uint32 targetWidth, uint32 targetHeight, uint32 targetDepth,
			uint32 channels)
		{
			imageResizeImpl<float>(
				sourceData, sourceWidth, sourceHeight, sourceDepth,
				targetData, targetWidth, targetHeight, targetDepth,
				channels);
		}
	}
}
