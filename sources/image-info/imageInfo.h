#ifndef guard_imageInfo_h_p4as44j5s5h4s
#define guard_imageInfo_h_p4as44j5s5h4s

#include <cage-core/image.h>
#include <cage-core/imageImport.h>

namespace cage
{
	inline void imageInfo(const ImageColorConfig &config)
	{
		if (config.alphaChannelIndex != m)
			CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "alpha channel index: " + config.alphaChannelIndex);
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "alpha mode: " + imageAlphaModeToString(config.alphaMode));
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "gamma space: " + imageGammaSpaceToString(config.gammaSpace));
	}

	inline void imageInfo(const Image *img)
	{
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "resolution: " + img->width() + "x" + img->height());
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "total channels: " + img->channels());
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "format: " + imageFormatToString(img->format()));
		imageInfo(img->colorConfig);
	}

	inline void imageInfo(const ImageImportRaw *raw)
	{
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "resolution: " + raw->resolution[0] + "x" + raw->resolution[1]);
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "total channels: " + raw->channels);
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "format: " + raw->format);
		imageInfo(raw->colorConfig);
	}

	inline void imageInfo(const ImageImportPart *part)
	{
		if (!part->fileName.empty())
			CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "file name: " + part->fileName);
		//if (!part->name.empty())
		//	CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "name: " + part->name);
		//CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "frame index: " + part->frameIndex);
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "mipmap level: " + part->mipmapLevel);
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "cube face: " + part->cubeFace);
		CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "layer: " + part->layer);
		if (part->image)
			imageInfo(+part->image);
		if (part->raw)
			imageInfo(+part->raw);
	}

	inline void imageInfo(const ImageImportResult &result)
	{
		switch (result.parts.size())
		{
			case 0:
				CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "loaded no images");
				break;
			case 1:
				break;
			default:
				CAGE_LOG(SeverityEnum::Info, "imageInfo", Stringizer() + "contains " + result.parts.size() + " subimages");
				break;
		}
		for (const auto &it : result.parts)
			imageInfo(&it);
	}
}

#endif // guard_imageInfo_h_p4as44j5s5h4s
