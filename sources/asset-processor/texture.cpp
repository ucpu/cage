#include <map>

#include "processor.h"

#include <cage-core/imageAlgorithms.h>
#include <cage-core/imageImport.h>
#include <cage-core/meshImport.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-engine/opengl.h>

void meshImportNotifyUsedFiles(const MeshImportResult &result);

namespace
{
	uint32 convertFilter(const String &f)
	{
		if (f == "nearestMipmapNearest")
			return GL_NEAREST_MIPMAP_NEAREST;
		if (f == "linearMipmapNearest")
			return GL_LINEAR_MIPMAP_NEAREST;
		if (f == "nearestMipmapLinear")
			return GL_NEAREST_MIPMAP_LINEAR;
		if (f == "linearMipmapLinear")
			return GL_LINEAR_MIPMAP_LINEAR;
		if (f == "nearest")
			return GL_NEAREST;
		if (f == "linear")
			return GL_LINEAR;
		return 0;
	}

	bool requireMipmaps(const uint32 f)
	{
		switch (f)
		{
			case GL_NEAREST_MIPMAP_NEAREST:
			case GL_LINEAR_MIPMAP_NEAREST:
			case GL_NEAREST_MIPMAP_LINEAR:
			case GL_LINEAR_MIPMAP_LINEAR:
				return true;
			default:
				return false;
		}
	}

	uint32 convertWrap(const String &f)
	{
		if (f == "clampToEdge")
			return GL_CLAMP_TO_EDGE;
		if (f == "clampToBorder")
			return GL_CLAMP_TO_BORDER;
		if (f == "mirroredRepeat")
			return GL_MIRRORED_REPEAT;
		if (f == "repeat")
			return GL_REPEAT;
		return 0;
	}

	uint32 convertTarget()
	{
		const String f = processor->property("target");
		if (f == "2d")
			return GL_TEXTURE_2D;
		if (f == "2dArray")
			return GL_TEXTURE_2D_ARRAY;
		if (f == "cubeMap")
			return GL_TEXTURE_CUBE_MAP;
		if (f == "3d")
			return GL_TEXTURE_3D;
		return 0;
	}

	uint32 findInternalFormatForBcn(const TextureHeader &data)
	{
		if (any(data.flags & TextureFlags::Srgb))
		{
			switch (data.channels)
			{
				case 3:
					return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
				case 4:
					return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
			}
		}
		else
		{
			switch (data.channels)
			{
				case 1:
					return GL_COMPRESSED_RED_RGTC1;
				case 2:
					return GL_COMPRESSED_RG_RGTC2;
				case 3:
					return GL_COMPRESSED_RGBA_BPTC_UNORM;
				case 4:
					return GL_COMPRESSED_RGBA_BPTC_UNORM;
			}
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	uint32 findInternalFormatForRaw(const TextureHeader &data)
	{
		if (any(data.flags & TextureFlags::Srgb))
		{
			switch (data.channels)
			{
				case 3:
					return GL_SRGB8;
				case 4:
					return GL_SRGB8_ALPHA8;
			}
		}
		else
		{
			switch (data.channels)
			{
				case 1:
					return GL_R8;
				case 2:
					return GL_RG8;
				case 3:
					return GL_RGB8;
				case 4:
					return GL_RGBA8;
			}
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	uint32 findCopyFormatForRaw(const TextureHeader &data)
	{
		switch (data.channels)
		{
			case 1:
				return GL_RED;
			case 2:
				return GL_RG;
			case 3:
				return GL_RGB;
			case 4:
				return GL_RGBA;
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	constexpr uint32 findContainedMipmapLevels(Vec3i res, bool is3d)
	{
		uint32 lvl = 1;
		while (res[0] > 1 || res[1] > 1 || (res[2] > 1 && is3d))
		{
			res /= 2;
			lvl++;
		}
		return lvl;
	}

	static_assert(findContainedMipmapLevels(Vec3i(), false) == 1);
	static_assert(findContainedMipmapLevels(Vec3i(1), false) == 1);
	static_assert(findContainedMipmapLevels(Vec3i(4, 1, 1), false) == 3);
	static_assert(findContainedMipmapLevels(Vec3i(4, 16, 1), false) == 5);
	static_assert(findContainedMipmapLevels(Vec3i(1, 1, 3), false) == 1);
	static_assert(findContainedMipmapLevels(Vec3i(1, 1, 3), true) == 2);

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

	void performDownscale(const uint32 downscale, const uint32 target)
	{
		if (target == GL_TEXTURE_3D)
		{ // downscale image as a whole
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "downscaling whole image (3D)");
			CAGE_THROW_ERROR(Exception, "3D texture downscale is not yet implemented");
		}
		else
		{ // downscale each image separately
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "downscaling each slice separately");
			for (auto &it : images.parts)
				imageResize(+it.image, max(it.image->width() / downscale, 1u), max(it.image->height() / downscale, 1u));
		}
	}

	void performSkyboxToCube()
	{
		if (images.parts.size() != 1)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires one input image");
		Holder<Image> src = std::move(images.parts[0].image);
		if (src->width() * 3 != src->height() * 4)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires source image to be 4:3");
		PointerRangeHolder<ImageImportPart> parts;
		parts.resize(6);
		uint32 sideIndex = 0;
		for (auto &tgt : parts)
		{
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
			switch (sideIndex++)
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
		}
		images.parts = std::move(parts);
	}

	void checkConsistency(const uint32 target)
	{
		const uint32 frames = numeric_cast<uint32>(images.parts.size());
		if (frames == 0)
			CAGE_THROW_ERROR(Exception, "no images were loaded");
		if (target == GL_TEXTURE_2D && frames > 1)
		{
			CAGE_LOG_THROW("did you forgot to set texture target?");
			CAGE_THROW_ERROR(Exception, "images have too many frames for a 2D texture");
		}
		if (target == GL_TEXTURE_CUBE_MAP && frames != 6)
			CAGE_THROW_ERROR(Exception, "cube texture requires exactly 6 images");
		if (target != GL_TEXTURE_2D && frames == 1)
			CAGE_LOG(SeverityEnum::Warning, "assetProcessor", "texture has only one frame. consider setting target to 2d");
		const Holder<Image> &im0 = images.parts[0].image;
		if (im0->width() == 0 || im0->height() == 0)
			CAGE_THROW_ERROR(Exception, "image has zero resolution");
		if (im0->channels() == 0 || im0->channels() > 4)
			CAGE_THROW_ERROR(Exception, "image has invalid bpp");
		for (auto &imi : images.parts)
		{
			if (imi.image->width() != im0->width() || imi.image->height() != im0->height())
				CAGE_THROW_ERROR(Exception, "frames has inconsistent resolutions");
			if (imi.image->channels() != im0->channels())
				CAGE_THROW_ERROR(Exception, "frames has inconsistent bpp");
		}
		if (target == GL_TEXTURE_CUBE_MAP && im0->width() != im0->height())
			CAGE_THROW_ERROR(Exception, "cube texture requires square textures");
	}

	void exportBcn(TextureHeader &data, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using bcn encoding");

		data.flags |= TextureFlags::Compressed;
		data.internalFormat = findInternalFormatForBcn(data);

		imageImportConvertImagesToBcn(images, toBool(processor->property("normal")));

		std::map<uint32, std::map<uint32, std::map<uint32, const ImageImportRaw *>>> levels;
		for (const auto &it : images.parts)
			levels[it.mipmapLevel][it.cubeFace][it.layer] = +it.raw;
		CAGE_ASSERT(levels.size() >= data.containedLevels);

		for (const auto &level : levels)
		{
			uint32 size = 0;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					size += layer.second->data.size();

			const uint32 layers = level.second.at(0).size();
			ser << Vec3i(level.second.at(0).at(0)->resolution, layers);
			ser << size;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					ser.write(layer.second->data);
		}
	}

	void exportRaw(TextureHeader &data, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, "assetProcessor", "using raw encoding - no compression");

		data.internalFormat = findInternalFormatForRaw(data);
		data.copyFormat = findCopyFormatForRaw(data);

		std::map<uint32, std::map<uint32, std::map<uint32, const Image *>>> levels;
		for (const auto &it : images.parts)
			levels[it.mipmapLevel][it.cubeFace][it.layer] = +it.image;
		CAGE_ASSERT(levels.size() >= data.containedLevels);

		for (const auto &level : levels)
		{
			uint32 size = 0;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					size += layer.second->rawViewU8().size();

			const uint32 layers = level.second.at(0).size();
			ser << Vec3i(level.second.at(0).at(0)->resolution(), layers);
			ser << size;
			for (const auto &face : level.second)
				for (const auto &layer : face.second)
					ser.write(bufferCast(layer.second->rawViewU8()));
		}
	}

	void exportTexture(const uint32 target)
	{
		TextureHeader data;
		detail::memset(&data, 0, sizeof(TextureHeader));
		data.target = target;
		data.resolution = Vec3i(images.parts[0].image->width(), images.parts[0].image->height(), numeric_cast<uint32>(images.parts.size()));
		data.channels = images.parts[0].image->channels();
		data.filterMin = convertFilter(processor->property("filterMin"));
		data.filterMag = convertFilter(processor->property("filterMag"));
		data.filterAniso = toUint32(processor->property("filterAniso"));
		data.wrapX = convertWrap(processor->property("wrapX"));
		data.wrapY = convertWrap(processor->property("wrapY"));
		data.wrapZ = convertWrap(processor->property("wrapZ"));
		data.swizzle[0] = TextureSwizzleEnum::R;
		data.swizzle[1] = TextureSwizzleEnum::G;
		data.swizzle[2] = TextureSwizzleEnum::B;
		data.swizzle[3] = TextureSwizzleEnum::A;
		data.containedLevels = requireMipmaps(data.filterMin) ? min(findContainedMipmapLevels(data.resolution, target == GL_TEXTURE_3D), 8u) : 1;
		data.mipmapLevels = data.containedLevels;
		if (toBool(processor->property("animationLoop")))
			data.flags |= TextureFlags::AnimationLoop;
		if (toBool(processor->property("srgb")) && !toBool(processor->property("gamma")))
			data.flags |= TextureFlags::Srgb;
		data.animationDuration = toUint64(processor->property("animationDuration"));
		data.copyType = GL_UNSIGNED_BYTE;

		// todo
		if (images.parts[0].image->format() != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(Exception, "8-bit precision only for now");

		if (data.containedLevels > 1)
			imageImportGenerateMipmaps(images);

		MemoryBuffer inputBuffer;

		{
			Serializer ser(inputBuffer);
			ser << data; // reserve space, will be overridden later

			const String compression = processor->property("compression");
			if (compression == "bcn")
				exportBcn(data, ser);
			else
				exportRaw(data, ser);
		}

		{ // overwrite the initial header
			Serializer ser(inputBuffer);
			ser << data;
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
		const bool h2n = processor->property("convert") == "heightToNormal";
		const bool s2s = processor->property("convert") == "specularToSpecial";
		const bool g2s = processor->property("convert") == "gltfToSpecial";
		const bool s2c = toBool(processor->property("skyboxToCube"));
		const bool pa = toBool(processor->property("premultiplyAlpha"));
		const bool srgb = toBool(processor->property("srgb"));
		const bool normal = toBool(processor->property("normal"));
		if ((h2n || s2s || g2s || normal) && pa)
			CAGE_THROW_ERROR(Exception, "premultiplied alpha is for colors only");
		if ((h2n || s2s || g2s || normal) && srgb)
			CAGE_THROW_ERROR(Exception, "srgb is for colors only");
		if ((s2s || g2s || pa || srgb) && normal)
			CAGE_THROW_ERROR(Exception, "incompatible options for normal map");
		if (h2n && !normal)
			CAGE_THROW_ERROR(Exception, "heightToNormal requires normal=true");
		if (s2c && processor->property("target") != "cubeMap")
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires target to be cubeMap");
		if (toBool(processor->property("gamma")) && !srgb)
			CAGE_THROW_ERROR(Exception, "sampling in gamma requires srgb color space");
	}

	loadAllFiles();

	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "input resolution: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
	CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "input channels: " + images.parts[0].image->channels());

	{ // change channels count
		const uint32 ch = toUint32(processor->property("channels"));
		if (ch != 0 && images.parts[0].image->channels() != ch)
		{
			for (auto &p : images.parts)
				imageConvert(+p.image, ch);
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

	{ // invert
		if (toBool(processor->property("invertRed")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 0);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "red channel inverted");
		}
		if (toBool(processor->property("invertGreen")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 1);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "green channel inverted");
		}
		if (toBool(processor->property("invertBlue")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 2);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "blue channel inverted");
		}
		if (toBool(processor->property("invertAlpha")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 3);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "alpha channel inverted");
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

	const uint32 target = convertTarget();
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
			performDownscale(downscale, target);
			CAGE_LOG(SeverityEnum::Info, "assetProcessor", Stringizer() + "downscaled: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
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
