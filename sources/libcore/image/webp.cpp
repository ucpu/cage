#include <webp/decode.h>
#include <webp/encode.h>

#include "image.h"
#include <cage-core/scopeGuard.h>

namespace cage
{
	void webpDecode(PointerRange<const char> inBuffer, ImageImpl *impl)
	{
		WebPBitstreamFeatures f;
		if (WebPGetFeatures((const std::uint8_t *)inBuffer.data(), inBuffer.size(), &f) != VP8_STATUS_OK)
			CAGE_THROW_ERROR(Exception, "invalid webp image");

		if (f.has_animation)
			CAGE_LOG(SeverityEnum::Warning, "cage", "decoding animations from webp is not yet supported");

		impl->initialize(f.width, f.height, f.has_alpha ? 4 : 3);

		bool result = false;
		if (f.has_alpha)
			result = WebPDecodeRGBAInto((const std::uint8_t *)inBuffer.data(), inBuffer.size(), (std::uint8_t *)impl->mem.data(), impl->mem.size(), impl->width * impl->channels);
		else
			result = WebPDecodeRGBInto((const std::uint8_t *)inBuffer.data(), inBuffer.size(), (std::uint8_t *)impl->mem.data(), impl->mem.size(), impl->width * impl->channels);
		if (!result)
			CAGE_THROW_ERROR(Exception, "webp decode failed");

		impl->colorConfig = defaultConfig(impl->channels);
	}

	MemoryBuffer webpEncode(const ImageImpl *impl)
	{
		if (impl->channels != 3 && impl->channels != 4)
			CAGE_THROW_ERROR(Exception, "unsupported channels count for webp encoding");
		if (impl->format != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "unsupported image format for webp encoding");

		std::uint8_t *webpData = nullptr;
		ScopeGuard _([&]() { WebPFree(webpData); });
		std::size_t webpSize = 0;
		switch (impl->channels)
		{
			case 3:
				webpSize = WebPEncodeLosslessRGB((const std::uint8_t *)impl->mem.data(), impl->width, impl->height, impl->width * 3, &webpData);
				break;
			case 4:
				webpSize = WebPEncodeLosslessRGBA((const std::uint8_t *)impl->mem.data(), impl->width, impl->height, impl->width * 4, &webpData);
				break;
		}
		if (webpSize == 0 || !webpData)
			CAGE_THROW_ERROR(Exception, "webp encode failed");

		MemoryBuffer res;
		res.resize(webpSize);
		detail::memcpy(res.data(), webpData, webpSize);
		return res;
	}
}
