#include <cage-core/imageImport.h>
#include <cage-core/meshImport.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-engine/opengl.h>

#include "processor.h"

#include <map>

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
		const String f = properties("target");
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
			case 3: return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
			case 4: return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
			}
		}
		else
		{
			switch (data.channels)
			{
			case 1: return GL_COMPRESSED_RED_RGTC1;
			case 2: return GL_COMPRESSED_RG_RGTC2;
			case 3: return GL_COMPRESSED_RGBA_BPTC_UNORM;
			case 4: return GL_COMPRESSED_RGBA_BPTC_UNORM;
			}
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	uint32 findInternalFormatForRaw(const TextureHeader &data)
	{
		if (toBool(properties("srgb")))
		{
			switch (data.channels)
			{
			case 3: return GL_SRGB8;
			case 4: return GL_SRGB8_ALPHA8;
			}
		}
		else
		{
			switch (data.channels)
			{
			case 1: return GL_R8;
			case 2: return GL_RG8;
			case 3: return GL_RGB8;
			case 4: return GL_RGBA8;
			}
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	uint32 findCopyFormatForRaw(const TextureHeader &data)
	{
		switch (data.channels)
		{
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	uint32 findContainedMipmapLevels(Vec3i res)
	{
		uint32 lvl = 1;
		while (res[0] > 1 || res[1] > 1 || res[2] > 1)
		{
			res /= 2;
			lvl++;
		}
		return lvl;
	}

	ImageImportResult images;

	MeshImportTexture *findEmbeddedTexture(const MeshImportResult &res)
	{
		for (const auto &itp : res.parts)
		{
			for (auto &itt : itp.textures)
			{
				if (isPattern(itt.name, "", "", String(Stringizer() + "?" + inputSpec)))
					return &itt;
			}
		}
		CAGE_THROW_ERROR(Exception, "requested embedded texture not found");
	}

	void loadAllFiles()
	{
		if (inputSpec.empty())
		{
			images = imageImportFiles(inputFileName);
			for (const auto &it : images.parts)
				writeLine(String("use=") + pathToRel(it.fileName, inputDirectory));
		}
		else
		{
			MeshImportResult res = meshImportFiles(inputFileName);
			meshImportNotifyUsedFiles(res);
			images = std::move(findEmbeddedTexture(res)->images);
		}
		imageImportConvertRawToImages(images);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "loading done");
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
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "downscaling whole image (3D)");
			CAGE_THROW_ERROR(NotImplemented, "3D texture downscale");
		}
		else
		{ // downscale each image separately
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "downscaling each slice separately");
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
			CAGE_LOG(SeverityEnum::Warning, logComponentName, "texture has only one frame. consider setting target to 2d");
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
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using bcn encoding");

		data.flags |= TextureFlags::Compressed;
		data.internalFormat = findInternalFormatForBcn(data);

		imageImportConvertImagesToBcn(images, toBool(properties("normal")));

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
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using raw encoding - no compression");

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
		data.filterMin = convertFilter(properties("filterMin"));
		data.filterMag = convertFilter(properties("filterMag"));
		data.filterAniso = toUint32(properties("filterAniso"));
		data.wrapX = convertWrap(properties("wrapX"));
		data.wrapY = convertWrap(properties("wrapY"));
		data.wrapZ = convertWrap(properties("wrapZ"));
		data.swizzle[0] = TextureSwizzleEnum::R;
		data.swizzle[1] = TextureSwizzleEnum::G;
		data.swizzle[2] = TextureSwizzleEnum::B;
		data.swizzle[3] = TextureSwizzleEnum::A;
		data.containedLevels = requireMipmaps(data.filterMin) ? min(findContainedMipmapLevels(data.resolution), 8u) : 1;
		data.maxMipmapLevel = data.containedLevels - 1;
		if (toBool(properties("animationLoop")))
			data.flags |= TextureFlags::AnimationLoop;
		if (toBool(properties("srgb")))
			data.flags |= TextureFlags::Srgb;
		data.animationDuration = toUint64(properties("animationDuration"));
		data.copyType = GL_UNSIGNED_BYTE;

		// todo
		if (images.parts[0].image->format() != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(NotImplemented, "8-bit precision only");

		if (data.containedLevels > 1)
			imageImportGenerateMipmaps(images);

		MemoryBuffer inputBuffer;

		{
			Serializer ser(inputBuffer);
			ser << data; // reserve space, will be overridden later

			const String compression = properties("compression");
			if (compression == "bcn")
				exportBcn(data, ser);
			else
				exportRaw(data, ser);
		}

		{ // overwrite the initial header
			Serializer ser(inputBuffer);
			ser << data;
		}

		AssetHeader h = initializeAssetHeader();
		h.originalSize = inputBuffer.size();
		Holder<PointerRange<char>> outputBuffer = compress(inputBuffer);
		h.compressedSize = outputBuffer.size();
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "final size: " + h.originalSize + ", compressed size: " + h.compressedSize + ", ratio: " + h.compressedSize / (float)h.originalSize);

		Holder<File> f = writeFile(outputFileName);
		f->write(bufferView(h));
		f->write(outputBuffer);
		f->close();
	}
}

void processTexture()
{
	{
		const bool cn = properties("convert") == "heightToNormal";
		const bool cs = properties("convert") == "specularToSpecial";
		const bool cc = properties("convert") == "skyboxToCube";
		const bool a = toBool(properties("premultiplyAlpha"));
		const bool s = toBool(properties("srgb"));
		const bool n = toBool(properties("normal"));
		if ((cn || cs || n) && a)
			CAGE_THROW_ERROR(Exception, "premultiplied alpha is for colors only");
		if ((cn || cs || n) && s)
			CAGE_THROW_ERROR(Exception, "srgb is for colors only");
		if ((cs || a || s) && n)
			CAGE_THROW_ERROR(Exception, "incompatible options for normal map");
		if (cn && !n)
			CAGE_THROW_ERROR(Exception, "heightToNormal requires normal=true");
		if (cc && properties("target") != "cubeMap")
			CAGE_THROW_ERROR(Exception, "convert skyboxToCube requires target to be cubeMap");
	}

	loadAllFiles();

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "input resolution: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "input channels: " + images.parts[0].image->channels());

	{ // invert
		if (toBool(properties("invertRed")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 0);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "red channel inverted");
		}
		if (toBool(properties("invertGreen")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 1);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "green channel inverted");
		}
		if (toBool(properties("invertBlue")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 2);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "blue channel inverted");
		}
		if (toBool(properties("invertAlpha")))
		{
			for (auto &it : images.parts)
				imageInvertChannel(+it.image, 3);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "alpha channel inverted");
		}
	}

	{ // convert height map to normal map
		if (properties("convert") == "heightToNormal")
		{
			const float strength = toFloat(properties("normalStrength"));
			for (auto &it : images.parts)
				imageConvertHeigthToNormal(+it.image, strength);
			overrideColorConfig(AlphaModeEnum::None, GammaSpaceEnum::Linear);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "converted from height map to normal map with strength of " + strength);
		}
	}

	{ // convert specular to special
		if (properties("convert") == "specularToSpecial")
		{
			for (auto &it : images.parts)
				imageConvertSpecularToSpecial(+it.image);
			overrideColorConfig(AlphaModeEnum::None, GammaSpaceEnum::Linear);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "converted specular colors to material special");
		}
	}

	{ // convert skybox to cube
		if (properties("convert") == "skyboxToCube")
		{
			performSkyboxToCube();
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "converted skybox to cube map");
		}
	}

	{ // convert to srgb
		if (toBool(properties("srgb")))
		{
			for (auto &it : images.parts)
				imageConvert(+it.image, GammaSpaceEnum::Gamma);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "converted to gamma space");
		}
		else
		{
			for (auto &it : images.parts)
				imageConvert(+it.image, GammaSpaceEnum::Linear);
			overrideColorConfig(AlphaModeEnum::None, GammaSpaceEnum::Linear);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "converted to linear space without alpha");
		}
	}

	const uint32 target = convertTarget();
	checkConsistency(target);

	{ // premultiply alpha
		if (toBool(properties("premultiplyAlpha")))
		{
			for (auto &it : images.parts)
			{
				if (it.image->channels() != 4)
					CAGE_THROW_ERROR(Exception, "premultiplied alpha requires 4 channels");
				imageConvert(+it.image, AlphaModeEnum::PremultipliedOpacity);
			}
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "premultiplied alpha");
		}
	}

	{ // downscale
		const uint32 downscale = toUint32(properties("downscale"));
		if (downscale > 1)
		{
			performDownscale(downscale, target);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "downscaled: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
		}
	}

	{ // normal map
		if (toBool(properties("normal")))
		{
			for (auto &it : images.parts)
			{
				switch (it.image->channels())
				{
				case 2: continue;
				case 3: break;
				default: CAGE_THROW_ERROR(Exception, "normal map requires 2 or 3 channels");
				}
				imageConvert(+it.image, 2);
			}
			overrideColorConfig(AlphaModeEnum::None, GammaSpaceEnum::Linear);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "made two-channel normal map");
		}
	}

	{ // vertical flip
		if (!toBool(properties("flip")))
		{
			for (auto &it : images.parts)
				imageVerticalFlip(+it.image);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "image vertically flipped (flip was false)");
		}
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "output resolution: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "output channels: " + images.parts[0].image->channels());

	exportTexture(target);

	if (configGetBool("cage-asset-processor/texture/preview"))
	{ // preview images
		imageImportConvertRawToImages(images); // this is after the export, so this operation does not affect the textures
		uint32 index = 0;
		for (auto &it : images.parts)
		{
			const String dbgName = pathJoin(configGetString("cage-asset-processor/texture/path", "asset-preview"), Stringizer() + pathReplaceInvalidCharacters(inputName) + "_" + it.mipmapLevel + "-" + it.cubeFace + "-" + it.layer + "_" + (index++) + ".png");
			imageVerticalFlip(+it.image);
			it.image->exportFile(dbgName);
		}
	}
}

void analyzeTexture()
{
	try
	{
		ImageImportResult res = imageImportFiles(pathJoin(inputDirectory, inputFile));
		if (res.parts.empty())
			CAGE_THROW_ERROR(Exception, "no images");
		writeLine("cage-begin");
		writeLine("scheme=texture");
		writeLine(String() + "asset=" + inputFile);
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}
