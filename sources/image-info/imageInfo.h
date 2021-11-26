#ifndef guard_imageInfo_h_p4as44j5s5h4s
#define guard_imageInfo_h_p4as44j5s5h4s

#include <cage-core/image.h>

namespace cage
{
	inline void imageInfo(const Image *img)
	{
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "resolution: " + img->width() + "x" + img->height());
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "total channels: " + img->channels());
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "color channels: " + img->colorConfig.colorChannelsCount);
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "gamma space: " + detail::imageGammaSpaceToString(img->colorConfig.gammaSpace));
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "alpha mode: " + detail::imageAlphaModeToString(img->colorConfig.alphaMode));
		if (img->colorConfig.alphaChannelIndex != m)
			CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "alpha channel index: " + img->colorConfig.alphaChannelIndex);
		CAGE_LOG(SeverityEnum::Info, "image", Stringizer() + "format: " + detail::imageFormatToString(img->format()));
	}
}

#endif // guard_imageInfo_h_p4as44j5s5h4s
