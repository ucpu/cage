#include <cage-core/image.h>
#include <cage-core/enumerate.h>
#include <cage-core/meshImport.h>
#include <cage-engine/opengl.h>

#include "processor.h"

#include <vector>
#include <set>

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

	uint32 convertTarget(const String &f)
	{
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

	std::vector<Holder<Image>> images;

	const MeshImportTexture *findEmbeddedTexture(const MeshImportResult &res, const String &spec)
	{
		for (const auto &itp : res.parts)
		{
			for (const auto &itt : itp.textures)
			{
				if (isPattern(itt.name, "", "", spec))
					return &itt;
			}
		}
		return nullptr;
	}

	bool loadEmbeddedTexture(const MeshImportResult &res, const String &spec, bool required)
	{
		const MeshImportTexture *tx = findEmbeddedTexture(res, spec);
		if (!tx)
		{
			if (required)
			{
				CAGE_LOG_THROW(Stringizer() + "embedded texture name: '" + spec + "'");
				CAGE_THROW_ERROR(Exception, "embedded texture not found");
			}
			return false;
		}
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "loaded embedded texture '" + spec + "'");
		images.push_back(tx->image.share());
		return true;
	}

	std::pair<bool, String> resolveDollars(const String &input, uint32 index)
	{
		const uint32 firstDollar = find(input, '$');
		if (firstDollar == m)
			return { false, input };

		String prefix = subString(input, 0, firstDollar);
		String suffix = subString(input, firstDollar, m);
		uint32 dollarsCount = 0;
		while (!suffix.empty() && suffix[0] == '$')
		{
			dollarsCount++;
			suffix = subString(suffix, 1, m);
		}
		const String name = prefix + reverse(fill(reverse(String(Stringizer() + index)), dollarsCount, '0')) + suffix;
		return { true, name };
	}

	void loadAllFiles()
	{
		for (uint32 ifi = 0;; ifi++)
		{
			const auto ifn = resolveDollars(inputFile, ifi);
			const String wholeFilename = pathJoin(inputDirectory, ifn.second);
			if (ifi > 0 && !pathIsFile(wholeFilename))
				break;
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "loading file '" + wholeFilename + "'");
			if (inputSpec.empty())
			{
				writeLine(String("use=") + ifn.second);
				Holder<Image> l = newImage();
				l->importFile(wholeFilename);
				images.push_back(std::move(l));
			}
			else
			{
				MeshImportConfig cfg;
				cfg.rootPath = inputDirectory;
				MeshImportResult res = meshImportFiles(wholeFilename, cfg);
				meshImportNotifyUsedFiles(res);
				for (uint32 isi = 0;; isi++)
				{
					const auto isn = resolveDollars(inputSpec, isi);
					if (!loadEmbeddedTexture(res, isn.second, isi == 0))
						break;
					if (!isn.first)
						break;
				}
			}
			if (!ifn.first)
				break;
		}
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "loading done");
	}

	void performDownscale(uint32 downscale, uint32 target)
	{
		if (target == GL_TEXTURE_3D)
		{ // downscale image as a whole
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "downscaling whole image (3D)");
			CAGE_THROW_ERROR(NotImplemented, "3D texture downscale");
		}
		else
		{ // downscale each image separately
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "downscaling each slice separately");
			for (auto &it : images)
				imageResize(it.get(), max(it->width() / downscale, 1u), max(it->height() / downscale, 1u));
		}
	}

	void performSkyboxToCube()
	{
		if (images.size() != 1)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires one input image");
		Holder<Image> src = std::move(images[0]);
		images.clear();
		images.resize(6);
		if (src->width() * 3 != src->height() * 4)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires source image to be 4:3");
		uint32 sideIndex = 0;
		for (auto &tgt : images)
		{
			tgt = newImage();
			tgt->initialize(src->width() / 4, src->height() / 3, src->channels(), src->format());
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
			xOffset *= tgt->width();
			yOffset *= tgt->height();
			imageBlit(+src, +tgt, xOffset, yOffset, 0, 0, tgt->width(), tgt->height());
		}
	}

	void checkConsistency(uint32 target)
	{
		const uint32 frames = numeric_cast<uint32>(images.size());
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
		const Holder<Image> &im0 = images[0];
		if (im0->width() == 0 || im0->height() == 0)
			CAGE_THROW_ERROR(Exception, "image has zero resolution");
		if (im0->channels() == 0 || im0->channels() > 4)
			CAGE_THROW_ERROR(Exception, "image has invalid bpp");
		for (auto &imi : images)
		{
			if (imi->width() != im0->width() || imi->height() != im0->height())
				CAGE_THROW_ERROR(Exception, "frames has inconsistent resolutions");
			if (imi->channels() != im0->channels())
				CAGE_THROW_ERROR(Exception, "frames has inconsistent bpp");
		}
		if (target == GL_TEXTURE_CUBE_MAP && im0->width() != im0->height())
			CAGE_THROW_ERROR(Exception, "cube texture requires square textures");
	}

	void exportTexture(uint32 target)
	{
		TextureHeader data;
		detail::memset(&data, 0, sizeof(TextureHeader));
		data.target = target;
		data.filterMin = convertFilter(properties("filterMin"));
		data.filterMag = convertFilter(properties("filterMag"));
		data.filterAniso = toUint32(properties("filterAniso"));
		data.wrapX = convertWrap(properties("wrapX"));
		data.wrapY = convertWrap(properties("wrapY"));
		data.wrapZ = convertWrap(properties("wrapZ"));
		data.flags =
			(requireMipmaps(data.filterMin) ? TextureFlags::GenerateMipmaps : TextureFlags::None) |
			(toBool(properties("animationLoop")) ? TextureFlags::AnimationLoop : TextureFlags::None);
		data.resolution = Vec3i(images[0]->width(), images[0]->height(), numeric_cast<uint32>(images.size()));
		data.channels = images[0]->channels();

		// todo
		if (images[0]->format() != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(NotImplemented, "8-bit precision only");

		if (toBool(properties("srgb")))
		{
			switch (data.channels)
			{
			case 3:
				data.internalFormat = GL_SRGB8;
				data.copyFormat = GL_RGB;
				break;
			case 4:
				data.internalFormat = GL_SRGB8_ALPHA8;
				data.copyFormat = GL_RGBA;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "unsupported bpp (with srgb)");
			}
		}
		else
		{
			switch (data.channels)
			{
			case 1:
				data.internalFormat = GL_R8;
				data.copyFormat = GL_RED;
				break;
			case 2:
				data.internalFormat = GL_RG8;
				data.copyFormat = GL_RG;
				break;
			case 3:
				data.internalFormat = GL_RGB8;
				data.copyFormat = GL_RGB;
				break;
			case 4:
				data.internalFormat = GL_RGBA8;
				data.copyFormat = GL_RGBA;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "unsupported bpp");
			}
		}
		data.copyType = GL_UNSIGNED_BYTE;
		data.stride = data.resolution[0] * data.resolution[1] * data.channels;
		data.animationDuration = toUint64(properties("animationDuration"));

		AssetHeader h = initializeAssetHeader();
		h.originalSize = sizeof(data);
		for (const auto &it : images)
			h.originalSize += it->rawViewU8().size();

		MemoryBuffer inputBuffer;
		inputBuffer.reserve(numeric_cast<uintPtr>(h.originalSize));
		Serializer ser(inputBuffer);
		ser << data;
		for (const auto &it : images)
			ser.write(bufferCast(it->rawViewU8()));

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
		const bool n = properties("convert") == "heightToNormal";
		const bool s = properties("convert") == "specularToSpecial";
		const bool c = properties("convert") == "skyboxToCube";
		const bool a = toBool(properties("premultiplyAlpha"));
		const bool g = toBool(properties("srgb"));
		if ((n || s) && a)
			CAGE_THROW_ERROR(Exception, "premultiplied alpha is only for colors");
		if ((n || s) && g)
			CAGE_THROW_ERROR(Exception, "srgb is only for colors");
		if (c && properties("target") != "cubeMap")
			CAGE_THROW_ERROR(Exception, "convert skyboxToCube requires target to be cubeMap");
	}

	loadAllFiles();

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "input resolution: " + images[0]->width() + "*" + images[0]->height() + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "input channels: " + images[0]->channels());

	{ // convert height map to normal map
		if (properties("convert") == "heightToNormal")
		{
			const float strength = toFloat(properties("normalStrength"));
			for (auto &it : images)
				imageConvertHeigthToNormal(+it, strength);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "converted from height map to normal map with strength of " + strength);
		}
	}

	{ // convert specular to special
		if (properties("convert") == "specularToSpecial")
		{
			for (auto &it : images)
				imageConvertSpecularToSpecial(+it);
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

	const uint32 target = convertTarget(properties("target"));
	checkConsistency(target);

	{ // premultiply alpha
		if (toBool(properties("premultiplyAlpha")))
		{
			for (auto &it : images)
			{
				if (it->channels() != 4)
					CAGE_THROW_ERROR(Exception, "premultiplied alpha requires 4 channels");
				if (toBool(properties("srgb")))
				{
					ImageFormatEnum origFormat = it->format();
					GammaSpaceEnum origGamma = it->colorConfig.gammaSpace;
					imageConvert(+it, ImageFormatEnum::Float);
					imageConvert(+it, GammaSpaceEnum::Linear);
					imageConvert(+it, AlphaModeEnum::PremultipliedOpacity);
					imageConvert(+it, origGamma);
					imageConvert(+it, origFormat);
				}
				else
					imageConvert(+it, AlphaModeEnum::PremultipliedOpacity);
			}
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "premultiplied alpha");
		}
	}

	{ // downscale
		const uint32 downscale = toUint32(properties("downscale"));
		if (downscale > 1)
		{
			performDownscale(downscale, target);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "downscaled: " + images[0]->width() + "*" + images[0]->height() + "*" + numeric_cast<uint32>(images.size()));
		}
	}

	{ // vertical flip
		if (!toBool(properties("flip")))
		{
			for (auto &it : images)
				imageVerticalFlip(+it);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "image vertically flipped (flip was false)");
		}
	}

	{ // invert
		if (toBool(properties("invertRed")))
		{
			for (auto &it : images)
				imageInvertChannel(+it, 0);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "red channel inverted");
		}
		if (toBool(properties("invertGreen")))
		{
			for (auto &it : images)
				imageInvertChannel(+it, 1);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "green channel inverted");
		}
		if (toBool(properties("invertBlue")))
		{
			for (auto &it : images)
				imageInvertChannel(+it, 2);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "blue channel inverted");
		}
		if (toBool(properties("invertAlpha")))
		{
			for (auto &it : images)
				imageInvertChannel(+it, 3);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "alpha channel inverted");
		}
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "output resolution: " + images[0]->width() + "*" + images[0]->height() + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "output channels: " + images[0]->channels());

	exportTexture(target);

	if (configGetBool("cage-asset-processor/texture/preview"))
	{ // preview images
		uint32 index = 0;
		for (auto &it : images)
		{
			String dbgName = pathJoin(configGetString("cage-asset-processor/texture/path", "asset-preview"), Stringizer() + pathReplaceInvalidCharacters(inputName) + "_" + (index++) + ".png");
			imageVerticalFlip(+it); // this is after the export, so this operation does not affect the textures
			it->exportFile(dbgName);
		}
	}
}

void analyzeTexture()
{
	try
	{
		Holder<Image> l = newImage();
		l->importFile(pathJoin(inputDirectory, inputFile));
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
