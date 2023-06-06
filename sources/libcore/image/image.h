#include <cage-core/image.h>
#include <cage-core/memoryBuffer.h>

namespace cage
{
	class ImageImpl : public Image
	{
	public:
		MemoryBuffer mem;
		uint32 width = 0, height = 0, channels = 0;
		ImageFormatEnum format = ImageFormatEnum::Default;
	};

	uint32 formatBytes(ImageFormatEnum format);

	ImageColorConfig defaultConfig(uint32 channels);

	void swapAll(ImageImpl *a, ImageImpl *b);
}
