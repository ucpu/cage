#include <cage-core/files.h>
#include <cage-core/imageImport.h>
#include <cage-core/imageBlocks.h>
#include <cage-core/pointerRangeHolder.h>

namespace cage
{
	namespace
	{
		void merge(ImageImportResult &inout, ImageImportResult &in)
		{
			PointerRangeHolder<ImageImportPart> tmp;
			tmp.reserve(inout.parts.size() + in.parts.size());
			for (auto &it : inout.parts)
				tmp.push_back(std::move(it));
			for (auto &it : in.parts)
				tmp.push_back(std::move(it));
			Holder<PointerRange<ImageImportPart>> tmp2 = std::move(tmp);
			std::swap(inout.parts, tmp2);
		}
	}

	ImageImportResult imageImportFiles(const String &filesPattern, const ImageImportConfig &config)
	{
		ImageImportResult result;
		const auto paths = pathSearchSequence(filesPattern);
		uint32 layer = 0;
		for (const String &p : paths)
		{
			Holder<File> f = readFile(p);
			Holder<PointerRange<char>> buffer = f->readAll();
			f->close();
			ImageImportResult tmp = imageImportBuffer(buffer, config);
			for (auto &it : tmp.parts)
				it.fileName = p;
			if (paths.size() > 1)
				for (auto &it : tmp.parts)
					it.layer = layer;
			merge(result, tmp);
			layer++;
		}
		return result;
	}

	void imageImportConvertRawToImages(ImageImportResult &result)
	{
		for (ImageImportPart &part : result.parts)
		{
			if (part.raw && !part.image)
			{
				ImageImportRaw &r = *+part.raw;
				if (r.format == "bc1")
					part.image = imageBc1Decode(r.data, r.resolution);
				else if (r.format == "bc2")
					part.image = imageBc2Decode(r.data, r.resolution);
				else if (r.format == "bc3")
					part.image = imageBc3Decode(r.data, r.resolution);
				else if (r.format == "bc4")
					part.image = imageBc4Decode(r.data, r.resolution);
				else if (r.format == "bc5")
					part.image = imageBc5Decode(r.data, r.resolution);
				else if (r.format == "bc7")
					part.image = imageBc7Decode(r.data, r.resolution);
				else
				{
					CAGE_LOG_THROW(Stringizer() + "format: " + r.format);
					CAGE_THROW_ERROR(Exception, "unknown format for raw-to-image conversion");
				}
				CAGE_ASSERT(part.image);
				part.image->colorConfig = r.colorConfig;
				part.raw.clear();
			}
		}
	}

	void imageImportConvertImagesToBcn(ImageImportResult &result, bool normals)
	{
		for (ImageImportPart &part : result.parts)
		{
			if (part.image && !part.raw)
			{
				ImageImportRaw r;
				r.colorConfig = part.image->colorConfig;
				r.resolution = part.image->resolution();
				r.channels = part.image->channels();
				switch (part.image->channels())
				{
				case 1:
					r.format = "bc4";
					r.data = imageBc4Encode(+part.image, { normals });
					break;
				case 2:
					r.format = "bc5";
					r.data = imageBc5Encode(+part.image, { normals });
					break;
				case 3:
					r.format = "bc7";
					r.data = imageBc7Encode(+part.image, { normals });
					break;
				case 4:
					r.format = "bc7";
					r.data = imageBc7Encode(+part.image, { normals });
					break;
				default:
					CAGE_THROW_ERROR(Exception, "unsupported number of channels for image-to-bcn conversion");
				}
				CAGE_ASSERT(r.data);
				part.raw = systemMemory().createHolder<ImageImportRaw>(std::move(r));
				part.image.clear();
			}
		}
	}

	void imageImportGenerateMipmaps(ImageImportResult &result)
	{
		imageImportConvertRawToImages(result);

		const auto &has = [&](uint32 level, uint32 face, uint32 layer) {
			for (const auto &it : result.parts)
				if (it.mipmapLevel == level && it.cubeFace == face && it.layer == layer)
					return true;
			return false;
		};

		PointerRangeHolder<ImageImportPart> parts;
		for (const auto &src : result.parts)
		{
			Vec2i resolution = src.image->resolution();
			Holder<Image> img = src.image.share();
			for (uint32 level = 1; ; level++)
			{
				resolution /= 2;
				resolution = max(resolution, 1);

				if (has(level, src.cubeFace, src.layer))
					continue;

				ImageImportPart p;
				p.fileName = src.fileName;
				p.mipmapLevel = level;
				p.cubeFace = src.cubeFace;
				p.layer = src.layer;
				p.image = img->copy();
				imageResize(+p.image, resolution);
				img = p.image.share();
				parts.push_back(std::move(p));

				if (resolution == Vec2i(1))
					break;
			}
		}

		for (auto &it : result.parts)
			parts.push_back(std::move(it));
		result.parts = std::move(parts);
	}
}
