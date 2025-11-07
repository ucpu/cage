#include <map>

#include <webgpu/webgpu_cpp.h>

#include "processor.h"

#include <cage-core/imageAlgorithms.h>
#include <cage-core/imageImport.h>
#include <cage-core/meshImport.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-engine/texture.h>

void meshImportNotifyUsedFiles(const MeshImportResult &result);

namespace
{
	wgpu::FilterMode convertFilter(const String &f)
	{
		if (f == "nearest")
			return wgpu::FilterMode::Nearest;
		if (f == "linear")
			return wgpu::FilterMode::Linear;
		return wgpu::FilterMode::Undefined;
	}

	wgpu::AddressMode convertWrap(const String &f)
	{
		if (f == "clamp")
			return wgpu::AddressMode::ClampToEdge;
		if (f == "repeat")
			return wgpu::AddressMode::Repeat;
		if (f == "mirror")
			return wgpu::AddressMode::MirrorRepeat;
		return wgpu::AddressMode::Undefined;
	}

	TextureFlags convertTarget()
	{
		TextureFlags result = TextureFlags::None;

		String target = processor->property("target");
		if (target == "regular")
			result |= TextureFlags::None;
		else if (target == "array")
			result |= TextureFlags::Array;
		else if (target == "cubeMap")
			result |= TextureFlags::Cubemap;
		else if (target == "cubeMapArray")
			result |= TextureFlags::Cubemap | TextureFlags::Array;
		else if (target == "volume")
			result |= TextureFlags::Volume3D;
		else
			CAGE_THROW_ERROR(Exception, "invalid target value");

		if (toBool(processor->property("srgb")))
			result |= TextureFlags::Srgb;
		if (toBool(processor->property("normal")))
			result |= TextureFlags::Normals;
		if (processor->property("compression") != "raw")
			result |= TextureFlags::Compressed;
		return result;
	}

	wgpu::TextureFormat findInternalFormatForBcn(const TextureHeader &data)
	{
		if (any(data.flags & TextureFlags::Srgb))
		{
			switch (data.channels)
			{
				case 3:
				case 4:
					return wgpu::TextureFormat::BC7RGBAUnormSrgb;
			}
		}
		else
		{
			switch (data.channels)
			{
				case 1:
					return wgpu::TextureFormat::BC4RUnorm;
				case 2:
					return wgpu::TextureFormat::BC5RGUnorm;
				case 3:
				case 4:
					return wgpu::TextureFormat::BC7RGBAUnorm;
			}
		}
		CAGE_THROW_ERROR(Exception, "invalid channels/srgb for compressed texture format");
	}

	wgpu::TextureFormat findInternalFormatForRaw(const TextureHeader &data)
	{
		if (any(data.flags & TextureFlags::Srgb))
		{
			switch (data.channels)
			{
				case 3:
				case 4:
					return wgpu::TextureFormat::RGBA8UnormSrgb;
			}
		}
		else
		{
			switch (data.channels)
			{
				case 1:
					return wgpu::TextureFormat::R8Unorm;
				case 2:
					return wgpu::TextureFormat::RG8Unorm;
				case 3:
				case 4:
					return wgpu::TextureFormat::RGBA8Unorm;
			}
		}
		CAGE_THROW_ERROR(Exception, "invalid channels/srgb for non-compressed texture format");
	}

	constexpr uint32 findContainedMipmapLevels(Vec3i res, bool volume3D, bool bcnCompression)
	{
		const uint32 mod = bcnCompression ? 4 : 1;
		uint32 lvl = 1;
		while (true)
		{
			if (res[0] <= 1 && res[1] <= 1 && (res[2] <= 1 || !volume3D))
				break;
			res /= 2;
			if ((res[0] % mod) != 0 || (res[1] % mod) != 0)
				break;
			lvl++;
		}
		return lvl;
	}

	static_assert(findContainedMipmapLevels(Vec3i(), false, false) == 1);
	static_assert(findContainedMipmapLevels(Vec3i(1), false, false) == 1);
	static_assert(findContainedMipmapLevels(Vec3i(4, 1, 1), false, false) == 3);
	static_assert(findContainedMipmapLevels(Vec3i(4, 16, 1), false, false) == 5);
	static_assert(findContainedMipmapLevels(Vec3i(1, 1, 3), false, false) == 1);
	static_assert(findContainedMipmapLevels(Vec3i(1, 1, 3), true, false) == 2);
	static_assert(findContainedMipmapLevels(Vec3i(1, 1, 1), false, true) == 1);
	static_assert(findContainedMipmapLevels(Vec3i(16, 16, 1), false, false) == 5);
	static_assert(findContainedMipmapLevels(Vec3i(16, 16, 1), false, true) == 3);

	ImageImportResult images;

	MeshImportTexture *findEmbeddedTexture(const MeshImportResult &res)
	{
		for (const auto &itp : res.parts)
		{
			for (auto &itt : itp.textures)
			{
				if (isPattern(itt.name, "", "", String(Stringizer() + "?" + processor->inputSpec)))
					return &itt;
			}
		}
		CAGE_THROW_ERROR(Exception, "requested embedded texture not found");
	}

	void loadAllFiles()
	{
		if (processor->inputSpec.empty())
		{
			images = imageImportFiles(processor->inputFileName);
			for (const auto &it : images.parts)
				processor->writeLine(String("use=") + pathToRel(it.fileName, processor->inputDirectory));
		}
		else
		{
			MeshImportConfig config;
			config.rootPath = processor->inputDirectory;
			config.verbose = true;
			MeshImportResult res = meshImportFiles(processor->inputFileName, config);

			//for (const auto &part : res.parts)
			//	for (const auto &it : part.textures)
			//		CAGE_LOG_CONTINUE(SeverityEnum::Info, "meshImport", Stringizer() + "texture: " + it.name + ", type: " + detail::meshImportTextureTypeToString(it.type) + ", parts: " + it.images.parts.size() + ", channels: " + (it.images.parts.empty() ? 0 : it.images.parts[0].image->channels()));

			CAGE_LOG(SeverityEnum::Info, "assetProcessor", "converting materials to cage format");
			meshImportConvertToCageFormats(res);

			//for (const auto &part : res.parts)
			//	for (const auto &it : part.textures)
			//		CAGE_LOG_CONTINUE(SeverityEnum::Info, "meshImport", Stringizer() + "texture: " + it.name + ", type: " + detail::meshImportTextureTypeToString(it.type) + ", parts: " + it.images.parts.size() + ", channels: " + (it.images.parts.empty() ? 0 : it.images.parts[0].image->channels()));

			meshImportNotifyUsedFiles(res);
			images = std::move(findEmbeddedTexture(res)->images);
		}
		imageImportConvertRawToImages(images);
		if (images.parts.empty())
			CAGE_THROW_ERROR(Exception, "loaded no images");
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "loading done");
	}

	void fillAlphaChannel(Image *img)
	{
		CAGE_ASSERT(img->channels() == 4);
		imageFill(img, 3, 1);
	}

	void overrideColorConfig(AlphaModeEnum alpha, GammaSpaceEnum gamma)
	{
		ImageColorConfig cfg;
		cfg.alphaMode = alpha;
		cfg.gammaSpace = gamma;
		if (alpha == AlphaModeEnum::None)
			cfg.alphaChannelIndex = m;
		else
			cfg.alphaChannelIndex = images.parts[0].image->channels() - 1;
		for (const auto &it : images.parts)
			it.image->colorConfig = cfg;
	}

	void performDownscale(const uint32 downscale, bool volume3D)
	{
		const auto &update = [&](uint32 w)
		{
			w = max(w / downscale, 1u);
			w = (w + 3) & ~3u; // round up to next multiple of 4
			return w;
		};

		if (volume3D)
		{ // downscale image as a whole
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "downscaling whole image (3D)");
			CAGE_THROW_ERROR(Exception, "volume 3D texture downscale is not yet implemented");
		}
		else
		{ // downscale each image separately
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "downscaling each slice separately");
			for (auto &it : images.parts)
				imageResize(+it.image, update(it.image->width()), update(it.image->height()));
		}
	}

	void performSkyboxToCube()
	{
		PointerRangeHolder<ImageImportPart> parts;
		parts.reserve(6 * parts.size());
		for (const ImageImportPart &input : images.parts)
		{
			const Holder<Image> &src = input.image;
			if (src->width() * 3 != src->height() * 4)
				CAGE_THROW_ERROR(Exception, "skyboxToCube requires source image to be 4:3");
			for (uint32 sideIndex = 0; sideIndex < 6; sideIndex++)
			{
				ImageImportPart tgt;
				tgt.layer = input.layer;
				tgt.cubeFace = sideIndex;
				tgt.image = newImage();
				tgt.image->initialize(src->width() / 4, src->height() / 3, src->channels(), src->format());
				/*
					 +---+
					 | 2 |
				 +---+---+---+---+
				 | 1 | 4 | 0 | 5 |
				 +---+---+---+---+
					 | 3 |
					 +---+
				*/
				uint32 yOffset = 0, xOffset = 0;
				switch (sideIndex)
				{
					case 0:
						xOffset = 2;
						yOffset = 1;
						break;
					case 1:
						xOffset = 0;
						yOffset = 1;
						break;
					case 2:
						xOffset = 1;
						yOffset = 0;
						break;
					case 3:
						xOffset = 1;
						yOffset = 2;
						break;
					case 4:
						xOffset = 1;
						yOffset = 1;
						break;
					case 5:
						xOffset = 3;
						yOffset = 1;
						break;
				}
				xOffset *= tgt.image->width();
				yOffset *= tgt.image->height();
				imageBlit(+src, +tgt.image, xOffset, yOffset, 0, 0, tgt.image->width(), tgt.image->height());
				parts.push_back(std::move(tgt));
			}
		}
		images.parts = std::move(parts);
		images.sort();
	}

	void checkConsistency(const TextureFlags target)
	{
		const uint32 frames = numeric_cast<uint32>(images.parts.size());
		if (frames == 0)
			CAGE_THROW_ERROR(Exception, "no images were loaded");

		if (any(target & TextureFlags::Cubemap))
		{
			if ((frames % 6) != 0)
				CAGE_THROW_ERROR(Exception, "cube texture requires multiple of 6 images");
			if (none(target & TextureFlags::Array) && frames != 6)
				CAGE_THROW_ERROR(Exception, "non-array cube texture requires exactly 6 images");
		}
		else if (none(target & TextureFlags::Array) && frames != 1)
		{
			CAGE_THROW_ERROR(Exception, "non-array texture requires exactly 1 image");
		}

		const Holder<Image> &im0 = images.parts[0].image;
		if (im0->width() == 0 || im0->height() == 0)
			CAGE_THROW_ERROR(Exception, "image has zero resolution");
		if (im0->channels() == 0 || im0->channels() > 4)
			CAGE_THROW_ERROR(Exception, "image has invalid channels count");

		for (auto &imi : images.parts)
		{
			if (imi.image->width() != im0->width() || imi.image->height() != im0->height())
				CAGE_THROW_ERROR(Exception, "frames have inconsistent resolutions");
			if (imi.image->channels() != im0->channels())
				CAGE_THROW_ERROR(Exception, "frames have inconsistent channels count");
		}

		if (any(target & TextureFlags::Cubemap) && im0->width() != im0->height())
			CAGE_THROW_ERROR(Exception, "cube texture requires square textures");
	}

	void exportBcn(TextureHeader &data, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using bcn encoding");

		if ((data.resolution[0] % 4) != 0 || (data.resolution[1] % 4) != 0)
			CAGE_THROW_ERROR(Exception, "base image resolution for bcn encoding must be divisible by 4");

		data.usage = (uint64)wgpu::TextureUsage::CopyDst | (uint64)wgpu::TextureUsage::TextureBinding;
		data.format = (uint32)findInternalFormatForBcn(data);

		imageImportConvertImagesToBcn(images, toBool(processor->property("normal")));

		std::map<uint32, std::map<uint32, std::map<uint32, const ImageImportRaw *>>> levels;
		for (const auto &it : images.parts)
			levels[it.mipmapLevel][it.cubeFace][it.layer] = +it.raw;
		CAGE_ASSERT(levels.size() >= data.mipLevels);

		for (const auto &level : levels)
		{
			if (level.first >= data.mipLevels)
				continue;

			uint32 size = 0;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					size += layer.second->data.size();

			const uint32 faces = level.second.size();
			const uint32 layers = level.second.at(0).size();
			ser << Vec3i(level.second.at(0).at(0)->resolution, faces * layers);
			ser << size;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					ser.write(layer.second->data);
		}
	}

	void exportRaw(TextureHeader &data, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using raw encoding - no compression");

		data.usage = (uint64)wgpu::TextureUsage::CopyDst | (uint64)wgpu::TextureUsage::TextureBinding;
		data.format = (uint32)findInternalFormatForRaw(data);

		std::map<uint32, std::map<uint32, std::map<uint32, const Image *>>> levels;
		for (const auto &it : images.parts)
			levels[it.mipmapLevel][it.cubeFace][it.layer] = +it.image;
		CAGE_ASSERT(levels.size() >= data.mipLevels);

		for (const auto &level : levels)
		{
			if (level.first >= data.mipLevels)
				continue;

			uint32 size = 0;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					size += layer.second->rawViewU8().size();

			const uint32 faces = level.second.size();
			const uint32 layers = level.second.at(0).size();
			ser << Vec3i(level.second.at(0).at(0)->resolution(), faces * layers);
			ser << size;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					ser.write(bufferCast(layer.second->rawViewU8()));
		}
	}

	void exportTexture(const TextureFlags target)
	{
		TextureHeader header;
		header.flags = target;
		header.resolution = Vec3i(images.parts[0].image->width(), images.parts[0].image->height(), numeric_cast<uint32>(images.parts.size()));
		header.channels = images.parts[0].image->channels();
		header.mipLevels = toBool(processor->property("mipmaps")) ? min(findContainedMipmapLevels(header.resolution, any(target & TextureFlags::Volume3D), any(target & TextureFlags::Compressed)), 8u) : 1;
		header.sampleFilter = (uint32)convertFilter(processor->property("sampleFilter"));
		header.mipmapFilter = (uint32)(header.mipLevels > 1 ? wgpu::MipmapFilterMode::Linear : wgpu::MipmapFilterMode::Nearest);
		header.anisoFilter = toUint32(processor->property("anisoFilter"));
		if (header.sampleFilter != (uint32)wgpu::FilterMode::Linear || header.mipmapFilter != (uint32)wgpu::MipmapFilterMode::Linear)
			header.anisoFilter = 1;
		header.wrapX = (uint32)convertWrap(processor->property("wrapX"));
		header.wrapY = (uint32)convertWrap(processor->property("wrapY"));
		header.wrapZ = (uint32)convertWrap(processor->property("wrapZ"));

		// todo
		if (images.parts[0].image->format() != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "8-bit precision only for now");

		if (header.mipLevels > 1)
			imageImportGenerateMipmaps(images);

		MemoryBuffer inputBuffer;

		{
			Serializer ser(inputBuffer);
			ser << header; // reserve space, will be overridden later

			const String compression = processor->property("compression");
			if (compression == "bcn")
				exportBcn(header, ser);
			else
				exportRaw(header, ser);
		}

		{ // overwrite the initial header
			Serializer ser(inputBuffer);
			ser << header;
		}

		AssetHeader h = processor->initializeAssetHeader();
		h.originalSize = inputBuffer.size();
		Holder<PointerRange<char>> outputBuffer = memoryCompress(inputBuffer);
		h.compressedSize = outputBuffer.size();
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "final size: " + h.originalSize + ", compressed size: " + h.compressedSize + ", ratio: " + h.compressedSize / (float)h.originalSize);

		Holder<File> f = writeFile(processor->outputFileName);
		f->write(bufferView(h));
		f->write(outputBuffer);
		f->close();
	}
}

void processTexture()
{
	{
		const String target = processor->property("target");
		const bool h2n = processor->property("convert") == "heightToNormal";
		const bool s2s = processor->property("convert") == "specularToSpecial";
		const bool g2s = processor->property("convert") == "gltfToSpecial";
		const bool s2c = toBool(processor->property("skyboxToCube"));
		const bool pa = toBool(processor->property("premultiplyAlpha"));
		const bool srgb = toBool(processor->property("srgb"));
		const bool normal = toBool(processor->property("normal"));
		//const bool mips = toBool(processor->property("mipmaps"));
		if ((h2n || s2s || g2s || normal) && pa)
			CAGE_THROW_ERROR(Exception, "premultiplied alpha is for colors only");
		if ((h2n || s2s || g2s || normal) && srgb)
			CAGE_THROW_ERROR(Exception, "srgb is for colors only");
		if ((s2s || g2s || pa || srgb) && normal)
			CAGE_THROW_ERROR(Exception, "incompatible options for normal map");
		if (h2n && !normal)
			CAGE_THROW_ERROR(Exception, "heightToNormal requires normal=true");
		const bool cubemapOrArray = target == "cubeMap" || target == "cubeMapArray";
		if (s2c && !cubemapOrArray)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires target to be cubeMap");
	}

	loadAllFiles();

	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "input resolution: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "input channels: " + images.parts[0].image->channels());

	{ // change channels count
		const uint32 ch = toUint32(processor->property("channels"));
		if (ch != 0 && images.parts[0].image->channels() != ch)
		{
			for (auto &p : images.parts)
			{
				imageConvert(+p.image, ch);
				if (ch == 4)
					fillAlphaChannel(+p.image);
			}
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converted to " + ch + " channels");
		}
	}

	{ // convert to srgb
		if (toBool(processor->property("srgb")))
		{
			for (auto &it : images.parts)
				imageConvert(+it.image, GammaSpaceEnum::Gamma);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converted to gamma space");
		}
		else
		{
			overrideColorConfig(AlphaModeEnum::None, GammaSpaceEnum::Linear);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "override to linear space (with alpha mode: none)");
		}
	}

	{ // vertical flip
		if (toBool(processor->property("flip")))
		{
			for (auto &it : images.parts)
				imageVerticalFlip(+it.image);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "image vertically flipped");
		}
	}

	{ // convert height to normal map
		if (processor->property("convert") == "heightToNormal")
		{
			const float strength = toFloat(processor->property("normalStrength"));
			for (auto &it : images.parts)
				imageConvertHeigthToNormal(+it.image, strength);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converted from height map to normal map with strength of " + strength);
		}
	}

	{ // convert specular to special
		if (processor->property("convert") == "specularToSpecial")
		{
			for (auto &it : images.parts)
				imageConvertSpecularToSpecial(+it.image);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converted specular colors to material special");
		}
	}

	{ // convert gltf pbr to special
		if (processor->property("convert") == "gltfToSpecial")
		{
			for (auto &it : images.parts)
				imageConvertGltfPbrToSpecial(+it.image);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converted gltf pbr to material special");
		}
	}

	{ // skybox to cube
		if (toBool(processor->property("skyboxToCube")))
		{
			performSkyboxToCube();
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converted skybox to cube map");
		}
	}

	const TextureFlags target = convertTarget();
	checkConsistency(target);

	{ // premultiply alpha
		if (toBool(processor->property("premultiplyAlpha")))
		{
			for (auto &it : images.parts)
			{
				if (it.image->channels() != 4)
					CAGE_THROW_ERROR(Exception, "premultiplied alpha requires 4 channels");
				imageConvert(+it.image, AlphaModeEnum::PremultipliedOpacity);
			}
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "premultiplied alpha");
		}
	}

	{ // normal map
		if (toBool(processor->property("normal")))
		{
			for (auto &it : images.parts)
			{
				switch (it.image->channels())
				{
					case 2:
						continue;
					case 3:
						break;
					default:
						CAGE_THROW_ERROR(Exception, "normal map requires 2 or 3 channels");
				}
				imageConvert(+it.image, 2);
			}
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "made two-channel normal map");
		}
	}

	{ // downscale
		const uint32 downscale = toUint32(processor->property("downscale"));
		if (downscale > 1)
		{
			performDownscale(downscale, any(target & TextureFlags::Volume3D));
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "downscaled: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
		}
	}

	{ // convert 3 channels to 4
		if (images.parts[0].image->channels() == 3)
		{
			CAGE_LOG(SeverityEnum::Warning, "assetProcessor", Stringizer() + "3-channel images are not supported, converting to 4 channels");
			for (auto &p : images.parts)
			{
				imageConvert(+p.image, 4);
				fillAlphaChannel(+p.image);
			}
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "converted to 4 channels");
		}
	}

	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "output resolution: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "output channels: " + images.parts[0].image->channels());

	exportTexture(target);

	if (configGetBool("cage-asset-processor/texture/preview"))
	{ // preview images
		imageImportConvertRawToImages(images); // this is after the export, so this operation does not affect the textures
		uint32 index = 0;
		for (auto &it : images.parts)
		{
			const String dbgName = pathJoin(configGetString("cage-asset-processor/texture/path", "asset-preview"), Stringizer() + pathReplaceInvalidCharacters(processor->inputName) + "_preview_mip_" + it.mipmapLevel + "_face_" + it.cubeFace + "_layer_" + it.layer + "_index_" + (index++) + ".png");
			it.image->exportFile(dbgName);
		}
	}
}

void analyzeTexture()
{
	try
	{
		ImageImportResult res = imageImportFiles(pathJoin(processor->inputDirectory, processor->inputFile));
		if (res.parts.empty())
			CAGE_THROW_ERROR(Exception, "no images");
		processor->writeLine("cage-begin");
		processor->writeLine("scheme=texture");
		processor->writeLine(String() + "asset=" + processor->inputFile);
		processor->writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}
