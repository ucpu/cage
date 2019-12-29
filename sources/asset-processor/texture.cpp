#include <vector>
#include <set>

#include "processor.h"

#include <cage-core/memoryBuffer.h>
#include <cage-core/image.h>
#include <cage-core/color.h>
#include <cage-core/serialization.h>
#include <cage-core/enumerate.h>

#include <cage-engine/opengl.h>

#include <IL/il.h>
#include <IL/ilu.h>

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

	ILenum convertBppToFormat(uint32 bpp)
	{
		switch (bpp)
		{
		case 1: return IL_LUMINANCE;
		case 2: return IL_LUMINANCE_ALPHA;
		case 3: return IL_RGB;
		case 4: return IL_RGBA;
		default:
			CAGE_THROW_ERROR(Exception, "unsupported bpp");
		}
	}

	struct imageLayerStruct
	{
		std::vector<uint8> data;
		uint32 width;
		uint32 height;
		uint32 bpp; // bytes per pixel

		void resizeDevil(uint32 depth = 1)
		{
			ilTexImage(width, height, depth, bpp, convertBppToFormat(bpp), IL_UNSIGNED_BYTE, nullptr);
		}

		void saveToDevil(uint32 depth = 0)
		{
			ilSetPixels(0, 0, depth, width, height, 1, convertBppToFormat(bpp), IL_UNSIGNED_BYTE, data.data());
		}

		void loadFromDevil(uint32 depth = 0)
		{
			width = ilGetInteger(IL_IMAGE_WIDTH);
			height = ilGetInteger(IL_IMAGE_HEIGHT);
			bpp = ilGetInteger(IL_IMAGE_BPP);
			data.resize(width * height * bpp);
			ilCopyPixels(0, 0, depth, width, height, 1, convertBppToFormat(bpp), IL_UNSIGNED_BYTE, data.data());
		}

		void performFlipV()
		{
			std::vector<uint8> tmp;
			uint32 line = width * bpp;
			tmp.resize(line);
			for (uint32 y = 0, yy = height / 2; y < yy; y++)
			{
				uint8 *a = data.data() + y * line;
				uint8 *b = data.data() + (height - 1 - y) * line;
				detail::memcpy(tmp.data(), a, line);
				detail::memcpy(a, b, line);
				detail::memcpy(b, tmp.data(), line);
			}
		}

		void convertHeightToNormal(float strength)
		{
			strength = 1.f / strength;
			std::vector<uint8> result;
			result.resize(width * height * 3);
			uint32 pos = 0;
			for (sint32 y = 0; (uint32)y < height; y++)
			{
				for (sint32 x = 0; (uint32)x < width; x++)
				{
					const float tl = convertToNormalIntensity(x - 1, y - 1);
					const float tc = convertToNormalIntensity(x + 0, y - 1);
					const float tr = convertToNormalIntensity(x + 1, y - 1);
					const float rc = convertToNormalIntensity(x + 1, y + 0);
					const float br = convertToNormalIntensity(x + 1, y + 1);
					const float bc = convertToNormalIntensity(x + 0, y + 1);
					const float bl = convertToNormalIntensity(x - 1, y + 1);
					const float lc = convertToNormalIntensity(x - 1, y + 0);
					const float dX = (tr + 2.f * rc + br) - (tl + 2.f * lc + bl);
					const float dY = (bl + 2.f * bc + br) - (tl + 2.f * tc + tr);
					vec3 v(-dX, -dY, strength);
					v = normalize(v);
					v += 1;
					v *= 0.5;
					v *= 255;
					result[pos + 0] = numeric_cast<uint8>(v[0]);
					result[pos + 1] = numeric_cast<uint8>(v[1]);
					result[pos + 2] = numeric_cast<uint8>(v[2]);
					pos += 3;
				}
			}
			std::swap(data, result);
			bpp = 3;
		}

		void convertSpecularToSpecial()
		{
			uint32 pixels = width * height;
			switch (bpp)
			{
			case 1:
			{
				for (uint32 i = 0; i < pixels; i++)
				{
					vec3 color = vec3(real(data[i]) / 255);
					vec2 special = ::convertSpecularToSpecial(color);
					CAGE_ASSERT(special[1] < 1e-7);
					data[i] = numeric_cast<uint8>(special[0] * 255);
				}
			} break;
			case 3:
			{
				std::vector<uint8> res;
				res.resize(pixels * 2);
				for (uint32 i = 0; i < pixels; i++)
				{
					vec3 color = vec3(data[i * 3 + 0], data[i * 3 + 1], data[i * 3 + 2]) / 255;
					vec2 special = ::convertSpecularToSpecial(color);
					res[i * 2 + 0] = numeric_cast<uint8>(special[0] * 255);
					res[i * 2 + 1] = numeric_cast<uint8>(special[1] * 255);
				}
				std::swap(res, data);
				bpp = 2;
			} break;
			default:
				CAGE_THROW_ERROR(Exception, "exactly 1 or 3 channels are required for conversion of specular color to special material");
			}
		}

		void invert(uint32 channelIndex)
		{
			if (channelIndex >= bpp)
				CAGE_THROW_ERROR(Exception, "texture does not have that channel");
			uint32 pixels = width * height;
			for (uint32 i = 0; i < pixels; i++)
				data[i * bpp + channelIndex] = 255 - data[i * bpp + channelIndex];
		}

		void premultiplyAlpha()
		{
			if (bpp != 4)
				CAGE_THROW_ERROR(Exception, "premultiplied alpha requires 4 components");
			if (properties("srgb").toBool())
				premultiplyAlphaImpl<true>();
			else
				premultiplyAlphaImpl<false>();
		}

	private:

		template<bool SRGB>
		void premultiplyAlphaImpl()
		{
			// todo depth
			uint32 line = width * bpp;
			for (uint32 y = 0; y < height; y++)
			{
				uint8 *l = data.data() + y * line;
				for (uint32 x = 0; x < width; x++)
				{
					uint8 *t = l + x * bpp;
					real a = t[3] / 255.f;
					for (uint32 i = 0; i < 3; i++)
					{
						if (SRGB)
						{
							real v = t[i] / 255.f;
							v = pow(v, 2.2f);
							v *= a; // alpha premultiplied in linear space
							v = pow(v, 1.0f / 2.2f);
							t[i] = numeric_cast<uint8>(v * 255.f);
						}
						else
							t[i] = numeric_cast<uint8>(a * t[i]);
					}
				}
			}
		}

		float convertToNormalIntensity(sint32 x, sint32 y)
		{
			x = min((sint32)width - 1, max(0, x));
			y = min((sint32)height - 1, max(0, y));
			uint32 pos = (y * width + x) * bpp;
			uint32 sum = 0;
			switch (bpp)
			{
			case 3: sum += data[pos + 2];
			case 2: sum += data[pos + 1];
			case 1: sum += data[pos + 0];
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid bpp");
			}
			return sum / 255.f / bpp;
		}
	};

	std::vector<imageLayerStruct> images;

	void loadSlice(uint32 index)
	{
		imageLayerStruct Image;
		Image.loadFromDevil(index);
		images.push_back(Image);
	}

	void loadFrame()
	{
		uint32 slices = ilGetInteger(IL_IMAGE_DEPTH);
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "slices count: " + slices);
		for (uint32 i = 0; i < slices; i++)
		{
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading slice: " + i);
			loadSlice(i);
		}
	}

	void loadFile(const string &filename)
	{
		writeLine(string("use=") + filename);
		ILuint im = ilGenImage();
		ilBindImage(im);
		{
			string wholeFilename = pathJoin(inputDirectory, filename);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading file '" + wholeFilename + "'");
			if (!ilLoadImage(wholeFilename.c_str()))
				CAGE_THROW_ERROR(Exception, "image format not supported");
		}
		uint32 frames = ilGetInteger(IL_NUM_IMAGES) + 1;
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "frames count: " + frames);
		for (uint32 i = 0; i < frames; i++)
		{
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading frame: " + i);
			ilBindImage(im);
			ilActiveImage(i);
			loadFrame();
		}
		ilBindImage(0);
		ilDeleteImage(im);
	}

	void performDownscale(uint32 downscale, uint32 target)
	{
		if (target == GL_TEXTURE_3D)
		{ // downscale image as a whole
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "downscaling whole image");
			ILuint im = ilGenImage();
			ilBindImage(im);
			iluImageParameter(ILU_FILTER, ILU_BILINEAR);
			images[0].resizeDevil(numeric_cast<uint32>(images.size()));
			//for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
			for (auto it : enumerate(images))
				it->saveToDevil(numeric_cast<uint32>(it.cnt));
			if (!iluScale(max(images[0].width / downscale, 1u), max(images[0].height / downscale, 1u), max(numeric_cast<uint32>(images.size()), 1u)))
				CAGE_THROW_ERROR(Exception, "iluScale");
			for (auto it : enumerate(images))
				it->loadFromDevil(numeric_cast<uint32>(it.cnt));
			ilBindImage(0);
			ilDeleteImage(im);
		}
		else
		{ // downscale each image separately
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "downscaling each slice separately");
			ILuint im = ilGenImage();
			ilBindImage(im);
			iluImageParameter(ILU_FILTER, ILU_BILINEAR);
			images[0].resizeDevil();
			for (auto it : enumerate(images))
			{
				CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "downscaling slice: " + it.cnt);
				it->saveToDevil();
				if (!iluScale(max(it->width / downscale, 1u), max(it->height / downscale, 1u), 1))
					CAGE_THROW_ERROR(Exception, "iluScale");
				it->loadFromDevil();
			}
			ilBindImage(0);
			ilDeleteImage(im);
		}
	}

	void performSkyboxToCube()
	{
		if (images.size() != 1)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires one input image");
		imageLayerStruct src = templates::move(images[0]);
		images.clear();
		images.resize(6);
		if (src.width * 3 != src.height * 4)
			CAGE_THROW_ERROR(Exception, "skyboxToCube requires source image to be 4:3");
		uint32 sideIndex = 0;
		for (auto &m : images)
		{
			m.width = src.width / 4;
			m.height = src.height / 3;
			m.bpp = src.bpp;
			m.data.resize(m.width * m.height * m.bpp);
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
			xOffset *= m.width;
			yOffset *= m.height;
			for (uint32 y = 0; y < m.height; y++)
			{
				const uint8 *s = src.data.data() + src.width * src.bpp * (y + yOffset) + src.bpp * xOffset;
				uint8 *t = m.data.data() + m.width * m.bpp * y;
				detail::memcpy(t, s, m.width * m.bpp);
			}
		}
	}

	void checkConsistency(uint32 target)
	{
		uint32 frames = numeric_cast<uint32>(images.size());
		if (frames == 0)
			CAGE_THROW_ERROR(Exception, "no images were loaded");
		if (target == GL_TEXTURE_2D && frames > 1)
		{
			CAGE_LOG(SeverityEnum::Note, "exception", "did you forgot to set texture target?");
			CAGE_THROW_ERROR(Exception, "images have too many frames for a 2D texture");
		}
		if (target == GL_TEXTURE_CUBE_MAP && frames != 6)
			CAGE_THROW_ERROR(Exception, "cube texture requires exactly 6 images");
		if (target != GL_TEXTURE_2D && frames == 1)
			CAGE_LOG(SeverityEnum::Warning, logComponentName, "texture has only one frame. consider setting target to 2d");
		imageLayerStruct &im0 = images[0];
		if (im0.width == 0 || im0.height == 0)
			CAGE_THROW_ERROR(Exception, "image has zero resolution");
		if (im0.bpp == 0 || im0.bpp > 4)
			CAGE_THROW_ERROR(Exception, "image has invalid bpp");
		for (auto &imi : images)
		{
			if (imi.width != im0.width || imi.height != im0.height)
				CAGE_THROW_ERROR(Exception, "frames has inconsistent resolutions");
			if (imi.bpp != im0.bpp)
				CAGE_THROW_ERROR(Exception, "frames has inconsistent bpp");
		}
		if (target == GL_TEXTURE_CUBE_MAP && im0.width != im0.height)
			CAGE_THROW_ERROR(Exception, "cube texture requires square textures");
	}

	void exportTexture(uint32 target)
	{
		TextureHeader data;
		detail::memset(&data, 0, sizeof(TextureHeader));
		data.target = target;
		data.filterMin = convertFilter(properties("filterMin"));
		data.filterMag = convertFilter(properties("filterMag"));
		data.filterAniso = properties("filterAniso").toUint32();
		data.wrapX = convertWrap(properties("wrapX"));
		data.wrapY = convertWrap(properties("wrapY"));
		data.wrapZ = convertWrap(properties("wrapZ"));
		data.flags =
			(requireMipmaps(data.filterMin) ? TextureFlags::GenerateMipmaps : TextureFlags::None) |
			(properties("animationLoop").toBool() ? TextureFlags::AnimationLoop : TextureFlags::None);
		data.dimX = images[0].width;
		data.dimY = images[0].height;
		data.dimZ = numeric_cast<uint32>(images.size());
		data.bpp = images[0].bpp;
		if (properties("srgb").toBool())
		{
			switch (data.bpp)
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
			switch (data.bpp)
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
		data.stride = data.dimX * data.dimY * data.bpp;
		data.animationDuration = properties("animationDuration").toUint64();

		AssetHeader h = initializeAssetHeaderStruct();
		h.originalSize = sizeof(data);
		for (const auto &it : images)
			h.originalSize += it.data.size();

		MemoryBuffer inputBuffer;
		inputBuffer.reserve(h.originalSize);
		Serializer ser(inputBuffer);
		ser << data;
		for (const auto &it : images)
			ser.write(it.data.data(), it.data.size());

		MemoryBuffer outputBuffer = detail::compress(inputBuffer);
		h.compressedSize = outputBuffer.size();
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "final size: " + h.originalSize + ", compressed size: " + h.compressedSize + ", ratio: " + h.compressedSize / (float)h.originalSize);

		Holder<File> f = newFile(outputFileName, FileMode(false, true));
		f->write(&h, sizeof(h));
		f->write(outputBuffer.data(), outputBuffer.size());
		f->close();
	}
}

void processTexture()
{
	{
		bool n = properties("convert") == "heightToNormal";
		bool s = properties("convert") == "specularToSpecial";
		bool c = properties("convert") == "skyboxToCube";
		bool a = properties("premultiplyAlpha").toBool();
		bool g = properties("srgb").toBool();
		if ((n || s) && a)
			CAGE_THROW_ERROR(Exception, "premultiplied alpha is only for colors");
		if ((n || s) && g)
			CAGE_THROW_ERROR(Exception, "srgb is only for colors");
		if (c && properties("target") != "cubeMap")
			CAGE_THROW_ERROR(Exception, "convert skyboxToCube requires target to be cubeMap");
	}

	ilInit();
	iluInit();
	ilEnable(IL_CONV_PAL);
	ilEnable(IL_FILE_OVERWRITE);

	{ // load all files
		uint32 firstDollar = inputFile.find('$');
		if (firstDollar == m)
			loadFile(inputFile);
		else
		{
			images.reserve(100);
			string prefix = inputFile.subString(0, firstDollar);
			string suffix = inputFile.subString(firstDollar, m);
			uint32 dollarsCount = 0;
			while (!suffix.empty() && suffix[0] == '$')
			{
				dollarsCount++;
				suffix = suffix.subString(1, m);
			}
			uint32 index = 0;
			while (true)
			{
				string name = prefix + string(index).reverse().fill(dollarsCount, '0').reverse() + suffix;
				if (!pathIsFile(pathJoin(inputDirectory, name)))
					break;
				CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading file: '" + name + "'");
				loadFile(name);
				index++;
			}
		}
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "loading done");
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "input resolution: " + images[0].width + "*" + images[0].height + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "input channels: " + images[0].bpp);

	{ // convert height map to normal map
		if (properties("convert") == "heightToNormal")
		{
			float strength = properties("normalStrength").toFloat();
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
		if (properties("premultiplyAlpha").toBool())
		{
			for (auto &it : images)
				it.premultiplyAlpha();
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "premultiplied alpha");
		}
	}

	{ // downscale
		uint32 downscale = properties("downscale").toUint32();
		if (downscale > 1)
		{
			performDownscale(downscale, target);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "downscaled: " + images[0].width + "*" + images[0].height + "*" + numeric_cast<uint32>(images.size()));
		}
	}

	{ // vertical flip
		if (!properties("flip").toBool())
		{
			for (auto &it : images)
				it.performFlipV();
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "image vertically flipped (flip was false)");
		}
	}

	{ // invert
		if (properties("invertRed").toBool())
		{
			for (auto &it : images)
				it.invert(0);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "red channel inverted");
		}
		if (properties("invertGreen").toBool())
		{
			for (auto &it : images)
				it.invert(1);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "green channel inverted");
		}
		if (properties("invertBlue").toBool())
		{
			for (auto &it : images)
				it.invert(2);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "blue channel inverted");
		}
		if (properties("invertAlpha").toBool())
		{
			for (auto &it : images)
				it.invert(3);
			CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "alpha channel inverted");
		}
	}

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "output resolution: " + images[0].width + "*" + images[0].height + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "output channels: " + images[0].bpp);

	exportTexture(target);

	if (configGetBool("cage-asset-processor/texture/preview"))
	{ // preview images
		uint32 index = 0;
		for (auto &it : images)
		{
			string dbgName = pathJoin(configGetString("cage-asset-processor/texture/path", "asset-preview"), stringizer() + pathReplaceInvalidCharacters(inputName) + "_" + (index++) + ".png");
			Holder<Image> png = newImage();
			png->empty(it.width, it.height, it.bpp);
			detail::memcpy(png->bufferData(), it.data.data(), png->bufferSize());
			png->verticalFlip();
			png->encodeFile(dbgName);
		}
	}

	ilShutDown();
}

void analyzeTexture()
{
	try
	{
		ilInit();
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
	ilShutDown();
}
