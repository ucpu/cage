#include <cage-core/image.h>
#include <cage-core/color.h>
#include <cage-core/enumerate.h>
#include <cage-engine/opengl.h>

#include "processor.h"

#include <vector>
#include <set>

vec2 convertSpecularToSpecial(const vec3 &spec)
{
	vec3 hsv = colorRgbToHsv(spec);
	return vec2(hsv[2], hsv[1]);
}

namespace
{
	uint32 convertFilter(const string &f)
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

	uint32 convertWrap(const string &f)
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

	uint32 convertTarget(const string &f)
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

	struct ImageLayer
	{
		Holder<Image> data;

		void convertHeightToNormal(float strength)
		{
			strength = 1.f / strength;
			uint32 w = data->width();
			uint32 h = data->height();
			Holder<Image> res = newImage();
			res->initialize(w, h, 3, data->format());
			for (sint32 y = 0; (uint32)y < h; y++)
			{
				for (sint32 x = 0; (uint32)x < w; x++)
				{
					const real tl = convertToNormalIntensity(x - 1, y - 1);
					const real tc = convertToNormalIntensity(x + 0, y - 1);
					const real tr = convertToNormalIntensity(x + 1, y - 1);
					const real rc = convertToNormalIntensity(x + 1, y + 0);
					const real br = convertToNormalIntensity(x + 1, y + 1);
					const real bc = convertToNormalIntensity(x + 0, y + 1);
					const real bl = convertToNormalIntensity(x - 1, y + 1);
					const real lc = convertToNormalIntensity(x - 1, y + 0);
					const real dX = (tr + 2.f * rc + br) - (tl + 2.f * lc + bl);
					const real dY = (bl + 2.f * bc + br) - (tl + 2.f * tc + tr);
					vec3 v(-dX, -dY, strength);
					v = normalize(v);
					v += 1;
					v *= 0.5;
					res->set(x, y, v);
				}
			}
			std::swap(data, res);
		}

		void convertSpecularToSpecial()
		{
			uint32 w = data->width();
			uint32 h = data->height();
			switch (data->channels())
			{
			case 1:
			{
				for (uint32 y = 0; y < h; y++)
				{
					for (uint32 x = 0; x < w; x++)
					{
						vec3 color = vec3(data->get1(x, y));
						vec2 special = ::convertSpecularToSpecial(color);
						CAGE_ASSERT(special[1] < 1e-7);
						data->set(x, y, special[0]);
					}
				}
			} break;
			case 3:
			{
				Holder<Image> res = newImage();
				res->initialize(w, h, 2, data->format());
				for (uint32 y = 0; y < h; y++)
				{
					for (uint32 x = 0; x < w; x++)
					{
						vec3 color = data->get3(x, y);
						vec2 special = ::convertSpecularToSpecial(color);
						res->set(x, y, special);
					}
				}
				std::swap(res, data);
			} break;
			default:
				CAGE_THROW_ERROR(Exception, "exactly 1 or 3 channels are required for conversion of specular color to special material");
			}
		}

		void invert(uint32 channelIndex)
		{
			if (channelIndex >= data->channels())
				CAGE_THROW_ERROR(Exception, "texture does not have that channel");
			uint32 w = data->width();
			uint32 h = data->height();
			for (uint32 y = 0; y < h; y++)
			{
				for (uint32 x = 0; x < w; x++)
					data->value(x, y, channelIndex, 1 - data->value(x, y, channelIndex));
			}
		}

		void premultiplyAlpha()
		{
			if (data->channels() != 4)
				CAGE_THROW_ERROR(Exception, "premultiplied alpha requires 4 channels");
			if (toBool(properties("srgb")))
			{
				ImageFormatEnum origFormat = data->format();
				GammaSpaceEnum origGamma = data->colorConfig.gammaSpace;
				imageConvert(data.get(), ImageFormatEnum::Float);
				imageConvert(data.get(), GammaSpaceEnum::Linear);
				imageConvert(data.get(), AlphaModeEnum::PremultipliedOpacity);
				imageConvert(data.get(), origGamma);
				imageConvert(data.get(), origFormat);
			}
			else
				imageConvert(data.get(), AlphaModeEnum::PremultipliedOpacity);
		}

	private:
		real convertToNormalIntensity(sint32 x, sint32 y)
		{
			x = min((sint32)data->width() - 1, max(0, x));
			y = min((sint32)data->height() - 1, max(0, y));
			real sum = 0;
			switch (data->channels())
			{
			case 3: sum += data->value(x, y, 2);
			case 2: sum += data->value(x, y, 1);
			case 1: sum += data->value(x, y, 0);
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid bpp");
			}
			return sum / data->channels();
		}
	};

	std::vector<ImageLayer> images;

	void loadFile(const string &filename)
	{
		writeLine(string("use=") + filename);
		string wholeFilename = pathJoin(inputDirectory, filename);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading file '" + wholeFilename + "'");
		ImageLayer l;
		l.data = newImage();
		l.data->importFile(wholeFilename);
		images.push_back(templates::move(l));
	}

	void performDownscale(uint32 downscale, uint32 target)
	{
		if (target == GL_TEXTURE_3D)
		{ // downscale image as a whole
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "downscaling whole image (3D)");
			CAGE_THROW_ERROR(NotImplemented, "3D texture downscale");
		}
		else
		{ // downscale each image separately
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "downscaling each slice separately");
			for (auto &it : images)
				imageResize(it.data.get(), max(it.data->width() / downscale, 1u), max(it.data->height() / downscale, 1u));
		}
	}

	void performSkyboxToCube()
	{
		if (images.size() != 1)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires one input image");
		ImageLayer src = templates::move(images[0]);
		images.clear();
		images.resize(6);
		if (src.data->width() * 3 != src.data->height() * 4)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires source image to be 4:3");
		uint32 sideIndex = 0;
		for (auto &m : images)
		{
			m.data = newImage();
			m.data->initialize(src.data->width() / 4, src.data->height() / 3, src.data->channels(), src.data->format());
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
			xOffset *= m.data->width();
			yOffset *= m.data->height();
			imageBlit(src.data.get(), m.data.get(), xOffset, yOffset, 0, 0, m.data->width(), m.data->height());
		}
	}

	void checkConsistency(uint32 target)
	{
		uint32 frames = numeric_cast<uint32>(images.size());
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
		ImageLayer &im0 = images[0];
		if (im0.data->width() == 0 || im0.data->height() == 0)
			CAGE_THROW_ERROR(Exception, "image has zero resolution");
		if (im0.data->channels() == 0 || im0.data->channels() > 4)
			CAGE_THROW_ERROR(Exception, "image has invalid bpp");
		for (auto &imi : images)
		{
			if (imi.data->width() != im0.data->width() || imi.data->height() != im0.data->height())
				CAGE_THROW_ERROR(Exception, "frames has inconsistent resolutions");
			if (imi.data->channels() != im0.data->channels())
				CAGE_THROW_ERROR(Exception, "frames has inconsistent bpp");
		}
		if (target == GL_TEXTURE_CUBE_MAP && im0.data->width() != im0.data->height())
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
		data.dimX = images[0].data->width();
		data.dimY = images[0].data->height();
		data.dimZ = numeric_cast<uint32>(images.size());
		data.channels = images[0].data->channels();

		// todo
		if (images[0].data->format() != ImageFormatEnum::U8)
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
		data.stride = data.dimX * data.dimY * data.channels;
		data.animationDuration = toUint64(properties("animationDuration"));

		AssetHeader h = initializeAssetHeader();
		h.originalSize = sizeof(data);
		for (const auto &it : images)
			h.originalSize += it.data->rawViewU8().size();

		MemoryBuffer inputBuffer;
		inputBuffer.reserve(h.originalSize);
		Serializer ser(inputBuffer);
		ser << data;
		for (const auto &it : images)
			ser.write(bufferCast(it.data->rawViewU8()));

		MemoryBuffer outputBuffer = detail::compress(inputBuffer);
		h.compressedSize = outputBuffer.size();
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "final size: " + h.originalSize + ", compressed size: " + h.compressedSize + ", ratio: " + h.compressedSize / (float)h.originalSize);

		Holder<File> f = newFile(outputFileName, FileMode(false, true));
		f->write(bufferView(h));
		f->write(outputBuffer);
		f->close();
	}
}

void processTexture()
{
	{
		bool n = properties("convert") == "heightToNormal";
		bool s = properties("convert") == "specularToSpecial";
		bool c = properties("convert") == "skyboxToCube";
		bool a = toBool(properties("premultiplyAlpha"));
		bool g = toBool(properties("srgb"));
		if ((n || s) && a)
			CAGE_THROW_ERROR(Exception, "premultiplied alpha is only for colors");
		if ((n || s) && g)
			CAGE_THROW_ERROR(Exception, "srgb is only for colors");
		if (c && properties("target") != "cubeMap")
			CAGE_THROW_ERROR(Exception, "convert skyboxToCube requires target to be cubeMap");
	}

	{ // load all files
		uint32 firstDollar = find(inputFile, '$');
		if (firstDollar == m)
			loadFile(inputFile);
		else
		{
			images.reserve(100);
			string prefix = subString(inputFile, 0, firstDollar);
			string suffix = subString(inputFile, firstDollar, m);
			uint32 dollarsCount = 0;
			while (!suffix.empty() && suffix[0] == '$')
			{
				dollarsCount++;
				suffix = subString(suffix, 1, m);
			}
			uint32 index = 0;
			while (true)
			{
				string name = prefix + reverse(fill(reverse(string(stringizer() + index)), dollarsCount, '0')) + suffix;
				if (!pathIsFile(pathJoin(inputDirectory, name)))
					break;
				CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading file: '" + name + "'");
				loadFile(name);
				index++;
			}
		}
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading done");
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "input resolution: " + images[0].data->width() + "*" + images[0].data->height() + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "input channels: " + images[0].data->channels());

	{ // convert height map to normal map
		if (properties("convert") == "heightToNormal")
		{
			float strength = toFloat(properties("normalStrength"));
			for (auto &it : images)
				it.convertHeightToNormal(strength);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "converted from height map to normal map with strength of " + strength);
		}
	}

	{ // convert specular to special
		if (properties("convert") == "specularToSpecial")
		{
			for (auto &it : images)
				it.convertSpecularToSpecial();
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "converted specular colors to material special");
		}
	}

	{ // convert skybox to cube
		if (properties("convert") == "skyboxToCube")
		{
			performSkyboxToCube();
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "converted skybox to cube map");
		}
	}

	uint32 target = convertTarget(properties("target"));
	checkConsistency(target);

	{ // premultiply alpha
		if (toBool(properties("premultiplyAlpha")))
		{
			for (auto &it : images)
				it.premultiplyAlpha();
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "premultiplied alpha");
		}
	}

	{ // downscale
		uint32 downscale = toUint32(properties("downscale"));
		if (downscale > 1)
		{
			performDownscale(downscale, target);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "downscaled: " + images[0].data->width() + "*" + images[0].data->height() + "*" + numeric_cast<uint32>(images.size()));
		}
	}

	{ // vertical flip
		if (!toBool(properties("flip")))
		{
			for (auto &it : images)
				imageVerticalFlip(+it.data);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "image vertically flipped (flip was false)");
		}
	}

	{ // invert
		if (toBool(properties("invertRed")))
		{
			for (auto &it : images)
				it.invert(0);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "red channel inverted");
		}
		if (toBool(properties("invertGreen")))
		{
			for (auto &it : images)
				it.invert(1);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "green channel inverted");
		}
		if (toBool(properties("invertBlue")))
		{
			for (auto &it : images)
				it.invert(2);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "blue channel inverted");
		}
		if (toBool(properties("invertAlpha")))
		{
			for (auto &it : images)
				it.invert(3);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "alpha channel inverted");
		}
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "output resolution: " + images[0].data->width() + "*" + images[0].data->height() + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "output channels: " + images[0].data->channels());

	exportTexture(target);

	if (configGetBool("cage-asset-processor/texture/preview"))
	{ // preview images
		uint32 index = 0;
		for (auto &it : images)
		{
			string dbgName = pathJoin(configGetString("cage-asset-processor/texture/path", "asset-preview"), stringizer() + pathReplaceInvalidCharacters(inputName) + "_" + (index++) + ".png");
			imageVerticalFlip(+it.data); // this is after the export, so this operation does not affect the textures
			it.data->exportFile(dbgName);
		}
	}
}

void analyzeTexture()
{
	try
	{
		loadFile(inputFile);
		writeLine("cage-begin");
		writeLine("scheme=texture");
		writeLine(string() + "asset=" + inputFile);
		writeLine("cage-end");
		images.clear();
	}
	catch (...)
	{
		// do nothing
	}
}
