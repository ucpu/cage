#include <cage-core/memoryBuffer.h>
#include <cage-core/image.h>

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
}
