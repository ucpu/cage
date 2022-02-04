#include <cage-core/image.h>
#include <cage-core/imageBlocks.h>
#include <cage-core/imageImport.h>
#include <cage-core/enumerate.h>
#include <cage-core/meshImport.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-engine/opengl.h>

#include "processor.h"

#include <vector>
#include <set>

void meshImportNotifyUsedFiles(const MeshImportResult &result);

namespace
{
#ifdef CAGE_DEBUG
	constexpr sint32 DefaltAstcCompressionQuality = 10;
#else
	constexpr sint32 DefaltAstcCompressionQuality = 70;
#endif // CAGE_DEBUG

	ConfigSint32 configAstcCompressionQuality("cage-asset-processor/texture/astcCompressionQuality", DefaltAstcCompressionQuality);

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

	String resolveAutoTiling(const uint32 target, const uint32 channels)
	{
		const String f = properties("astcTiling");
		if (f != "auto")
			return f;

		switch (target)
		{
		case GL_TEXTURE_2D:
		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_CUBE_MAP:
			if (toBool(properties("srgb")))
			{
				// approx 1 bit per pixel per channel
				switch (channels)
				{
				case 1: return "12x10";
				case 2: return "8x8";
				case 3: return "8x5";
				case 4: return "6x5";
				}
			}
			else
			{
				// approx 2 bits per pixel per channel
				switch (channels)
				{
				case 1: return "8x8";
				case 2: return "6x5";
				case 3: return "5x4";
				case 4: return "4x4";
				}
			}
		}

		CAGE_THROW_ERROR(Exception, "could not automatically determine astc tiling");
	}

	Vec2i convertTiling(const String &f)
	{
		String s = f;
		const uint32 x = toUint32(split(s, "x"));
		const uint32 y = toUint32(s);
		return Vec2i(x, y);
	}

	uint32 findInternalFormatForAstc(const Vec2i &tiling)
	{
		const uint32 t = tiling[0] * 100 + tiling[1]; // 8x5 -> 805

		if (toBool(properties("srgb")))
		{
			switch (t)
			{
			case 404: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR;
			case 504: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR;
			case 505: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR;
			case 605: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR;
			case 606: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR;
			case 805: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR;
			case 806: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR;
			case 808: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR;
			case 1005: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR;
			case 1006: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR;
			case 1008: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR;
			case 1010: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR;
			case 1210: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR;
			case 1212: return GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR;
			}
		}
		else
		{
			switch (t)
			{
			case 404: return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
			case 504: return GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
			case 505: return GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
			case 605: return GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
			case 606: return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
			case 805: return GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
			case 806: return GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
			case 808: return GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
			case 1005: return GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
			case 1006: return GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
			case 1008: return GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
			case 1010: return GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
			case 1210: return GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
			case 1212: return GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
			}
		}

		CAGE_THROW_ERROR(Exception, "cannot determine internal format for astc compressed texture");
	}

	uint32 findInternalFormatForBcn(const TextureHeader &data)
	{
		if (any(data.flags & TextureFlags::Srgb))
		{
			switch (data.channels)
			{
			case 3: return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
			case 4: return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
			}
		}
		else
		{
			switch (data.channels)
			{
			case 1: return GL_COMPRESSED_RED_RGTC1;
			case 2: return GL_COMPRESSED_RG_RGTC2;
			case 3: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			case 4: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	ImageKtxTranscodeFormatEnum findBlockFormat(const TextureHeader &data)
	{
		switch (data.channels)
		{
		case 1: return ImageKtxTranscodeFormatEnum::Bc4;
		case 2: return ImageKtxTranscodeFormatEnum::Bc5;
		case 3: return ImageKtxTranscodeFormatEnum::Bc1;
		case 4: return ImageKtxTranscodeFormatEnum::Bc3;
		}
		CAGE_THROW_ERROR(Exception, "invalid number of channels in texture");
	}

	void findSwizzlingForAstc(TextureSwizzleEnum swizzle[4], const uint32 channels)
	{
		if (toBool(properties("srgb")))
			return; // no swizzling

		switch (channels)
		{
		case 1:
			swizzle[0] = TextureSwizzleEnum::R;
			swizzle[1] = TextureSwizzleEnum::Zero;
			swizzle[2] = TextureSwizzleEnum::Zero;
			swizzle[3] = TextureSwizzleEnum::One;
			return;
		case 2:
			swizzle[0] = TextureSwizzleEnum::R;
			swizzle[1] = TextureSwizzleEnum::A;
			swizzle[2] = TextureSwizzleEnum::Zero;
			swizzle[3] = TextureSwizzleEnum::One;
			return;
		case 3:
			swizzle[0] = TextureSwizzleEnum::R;
			swizzle[1] = TextureSwizzleEnum::G;
			swizzle[2] = TextureSwizzleEnum::B;
			swizzle[3] = TextureSwizzleEnum::One;
			return;
		case 4:
			// no swizzling
			return;
		}

		CAGE_THROW_ERROR(Exception, "cannot determine swizzling for astc compressed texture");
	}

	ImageImportResult images;

	MeshImportTexture *findEmbeddedTexture(const MeshImportResult &res, const String &spec)
	{
		for (const auto &itp : res.parts)
		{
			for (auto &itt : itp.textures)
			{
				if (isPattern(itt.name, "", "", spec))
					return &itt;
			}
		}
		return nullptr;
	}

	void loadAllFiles()
	{
		const String wholeFilename = pathJoin(inputDirectory, inputFile);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "loading file: " + wholeFilename);
		if (inputSpec.empty())
		{
			images = imageImportFiles(wholeFilename);
			for (const auto &it : images.parts)
				writeLine(String("use=") + pathToRel(it.fileName, inputDirectory));
		}
		else
		{
			MeshImportConfig cfg;
			cfg.rootPath = inputDirectory;
			MeshImportResult res = meshImportFiles(wholeFilename, cfg);
			meshImportNotifyUsedFiles(res);
			images = std::move(findEmbeddedTexture(res, inputSpec)->images);
		}
		imageImportConvertRawToImages(images);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "loading done");
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

	void exportKtx(TextureHeader &data, AssetHeader &asset, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using ktx encoding");

		ImageKtxEncodeConfig cfg;
		//cfg.cubemap = data.target == GL_TEXTURE_CUBE_MAP;
		cfg.normals = toBool(properties("normal"));

		data.flags |= TextureFlags::Ktx;

		std::vector<const Image *> imgs;
		imgs.reserve(images.parts.size());
		for (const auto &it : images.parts)
			imgs.push_back(+it.image);
		auto ktx = imageKtxEncode(imgs, cfg);
		asset.originalSize += ktx.size();
		ser.write(ktx);
	}

	void exportBcn(TextureHeader &data, AssetHeader &asset, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using bcn encoding");

		data.flags |= TextureFlags::Compressed;
		data.internalFormat = findInternalFormatForBcn(data);

		ImageKtxEncodeConfig cfg1;
		//cfg1.cubemap = data.target == GL_TEXTURE_CUBE_MAP;
		cfg1.normals = toBool(properties("normal"));
		ImageKtxTranscodeConfig cfg2;
		cfg2.format = findBlockFormat(data);

		std::vector<const Image *> imgs;
		imgs.reserve(images.parts.size());
		for (const auto &it : images.parts)
			imgs.push_back(+it.image);
		auto trs = imageKtxTranscode(imgs, cfg1, cfg2);

		for (const auto &it : trs)
		{
			asset.originalSize += it.data.size();
			ser.write(it.data);
		}
	}

	void exportAstc(TextureHeader &data, AssetHeader &asset, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using astc encoding");
		const String astcTilingStr = resolveAutoTiling(data.target, data.channels);
		CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "astc tiling: " + astcTilingStr);

		ImageAstcEncodeConfig cfg;
		cfg.tiling = convertTiling(astcTilingStr);
		cfg.quality = configAstcCompressionQuality;
		cfg.normals = toBool(properties("normal"));

		data.flags |= TextureFlags::Compressed;
		data.internalFormat = findInternalFormatForAstc(cfg.tiling);
		findSwizzlingForAstc(data.swizzle, data.channels);

		for (const auto &it : images.parts)
		{
			auto astc = imageAstcEncode(+it.image, cfg);
			asset.originalSize += astc.size();
			ser.write(astc);
		}
	}

	void exportRaw(TextureHeader &data, AssetHeader &asset, Serializer &ser)
	{
		CAGE_LOG(SeverityEnum::Info, logComponentName, "using raw encoding - no compression");

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

		for (const auto &it : images.parts)
		{
			asset.originalSize += it.image->rawViewU8().size();
			ser.write(bufferCast(it.image->rawViewU8()));
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
		if (requireMipmaps(data.filterMin))
			data.flags |= TextureFlags::GenerateMipmaps;
		if (toBool(properties("animationLoop")))
			data.flags |= TextureFlags::AnimationLoop;
		if (toBool(properties("srgb")))
			data.flags |= TextureFlags::Srgb;
		data.animationDuration = toUint64(properties("animationDuration"));
		data.copyType = GL_UNSIGNED_BYTE;

		// todo
		if (images.parts[0].image->format() != ImageFormatEnum::U8)
			CAGE_THROW_ERROR(NotImplemented, "8-bit precision only");

		AssetHeader h = initializeAssetHeader();
		h.originalSize = sizeof(data);

		MemoryBuffer inputBuffer;
		Serializer ser(inputBuffer);
		ser << data; // reserve space, will be overridden later

		const String compression = properties("compression");
		if (compression == "bcn")
			exportBcn(data, h, ser);
		else if (compression == "ktx")
			exportKtx(data, h, ser);
		else if (compression == "astc")
			exportAstc(data, h, ser);
		else
			exportRaw(data, h, ser);

		{
			Serializer ser(inputBuffer);
			ser << data; // overwrite the initial header
		}

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

	{ // convert height map to normal map
		if (properties("convert") == "heightToNormal")
		{
			const float strength = toFloat(properties("normalStrength"));
			for (auto &it : images.parts)
				imageConvertHeigthToNormal(+it.image, strength);
			CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "converted from height map to normal map with strength of " + strength);
		}
	}

	{ // convert specular to special
		if (properties("convert") == "specularToSpecial")
		{
			for (auto &it : images.parts)
				imageConvertSpecularToSpecial(+it.image);
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

	const uint32 target = convertTarget();
	checkConsistency(target);

	{ // premultiply alpha
		if (toBool(properties("premultiplyAlpha")))
		{
			for (auto &it : images.parts)
			{
				if (it.image->channels() != 4)
					CAGE_THROW_ERROR(Exception, "premultiplied alpha requires 4 channels");
				if (toBool(properties("srgb")))
				{
					const ImageFormatEnum origFormat = it.image->format();
					const GammaSpaceEnum origGamma = it.image->colorConfig.gammaSpace;
					imageConvert(+it.image, ImageFormatEnum::Float);
					imageConvert(+it.image, GammaSpaceEnum::Linear);
					imageConvert(+it.image, AlphaModeEnum::PremultipliedOpacity);
					imageConvert(+it.image, origGamma);
					imageConvert(+it.image, origFormat);
				}
				else
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

	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "output resolution: " + images.parts[0].image->width() + "*" + images.parts[0].image->height() + "*" + numeric_cast<uint32>(images.parts.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, Stringizer() + "output channels: " + images.parts[0].image->channels());

	exportTexture(target);

	if (configGetBool("cage-asset-processor/texture/preview"))
	{ // preview images
		// this is after the export, so this operation does not affect the textures
		uint32 index = 0;
		for (auto &it : images.parts)
		{
			const String dbgName = pathJoin(configGetString("cage-asset-processor/texture/path", "asset-preview"), Stringizer() + pathReplaceInvalidCharacters(inputName) + "_" + (index++) + ".png");
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
