#include <vector>
#include <set>

#include "processor.h"

#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/png.h>

#include <cage-client/opengl.h>

#include <IL/il.h>
#include <IL/ilu.h>

namespace
{
	const uint32 convertFilter(const string &f)
	{
		if (f == "GL_NEAREST_MIPMAP_NEAREST")
			return GL_NEAREST_MIPMAP_NEAREST;
		if (f == "GL_LINEAR_MIPMAP_NEAREST")
			return GL_LINEAR_MIPMAP_NEAREST;
		if (f == "GL_NEAREST_MIPMAP_LINEAR")
			return GL_NEAREST_MIPMAP_LINEAR;
		if (f == "GL_LINEAR_MIPMAP_LINEAR")
			return GL_LINEAR_MIPMAP_LINEAR;
		if (f == "GL_NEAREST")
			return GL_NEAREST;
		if (f == "GL_LINEAR")
			return GL_LINEAR;
		return 0;
	}

	const bool requireMipmaps(const uint32 f)
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

	const uint32 convertWrap(const string &f)
	{
		if (f == "GL_CLAMP_TO_EDGE")
			return GL_CLAMP_TO_EDGE;
		if (f == "GL_CLAMP_TO_BORDER")
			return GL_CLAMP_TO_BORDER;
		if (f == "GL_MIRRORED_REPEAT")
			return GL_MIRRORED_REPEAT;
		if (f == "GL_REPEAT")
			return GL_REPEAT;
		return 0;
	}

	const uint32 convertTarget(const string &f)
	{
		if (f == "GL_TEXTURE_2D")
			return GL_TEXTURE_2D;
		if (f == "GL_TEXTURE_2D_ARRAY")
			return GL_TEXTURE_2D_ARRAY;
		if (f == "GL_TEXTURE_CUBE_MAP")
			return GL_TEXTURE_CUBE_MAP;
		if (f == "GL_TEXTURE_3D")
			return GL_TEXTURE_3D;
		return 0;
	}

	const ILenum convertBppToFormat(uint32 bpp)
	{
		switch (bpp)
		{
		case 1: return IL_LUMINANCE;
		case 2: return IL_LUMINANCE_ALPHA;
		case 3: return IL_RGB;
		case 4: return IL_RGBA;
		default:
			CAGE_THROW_ERROR(exception, "unsupported bpp");
		}
	}

	struct imageLayerStruct
	{
		std::vector<uint8> data;
		uint32 width;
		uint32 height;
		uint32 bpp; // bytes per pixel

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

		void performFlipv()
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
			std::vector<unsigned char> result;
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
					vec3 v(dX, dY, strength);
					v = v.normalize();
					v += 1;
					v *= 0.5;
					v *= 255;
					result[pos + 0] = numeric_cast<unsigned char>(v[0]);
					result[pos + 1] = numeric_cast<unsigned char>(v[1]);
					result[pos + 2] = numeric_cast<unsigned char>(v[2]);
					pos += 3;
				}
			}
			std::swap(data, result);
			bpp = 3;
		}

		void premultiplyAlpha()
		{
			if (bpp != 4)
				return;
			uint32 line = width * bpp;
			for (uint32 y = 0; y < height; y++)
			{
				uint8 *l = data.data() + y * line;
				for (uint32 x = 0; x < width; x++)
				{
					uint8 *t = l + x * bpp;
					float f = t[3] / 255.f;
					for (uint32 i = 0; i < 3; i++)
						t[i] = numeric_cast<uint8>(f * t[i]);
				}
			}
		}

	private:
		const float convertToNormalIntensity(sint32 x, sint32 y)
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
				CAGE_THROW_CRITICAL(exception, "invalid bpp");
			}
			return sum / 256.f / bpp;
		}
	};

	std::vector<imageLayerStruct> images;

	void loadSlice(uint32 index)
	{
		imageLayerStruct image;
		image.loadFromDevil(index);
		images.push_back(image);
	}

	void loadFrame()
	{
		uint32 slices = ilGetInteger(IL_IMAGE_DEPTH);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "slices count: " + slices);
		for (uint32 i = 0; i < slices; i++)
		{
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "loading slice: " + i);
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
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "loading file '" + wholeFilename + "'");
			if (!ilLoadImage(wholeFilename.c_str()))
				CAGE_THROW_ERROR(exception, "image format not supported");
		}
		uint32 frames = ilGetInteger(IL_NUM_IMAGES) + 1;
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "frames count: " + frames);
		for (uint32 i = 0; i < frames; i++)
		{
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "loading frame: " + i);
			ilBindImage(im);
			ilActiveImage(i);
			loadFrame();
		}
		ilBindImage(0);
		ilDeleteImage(im);
	}

	void performDownscale(uint32 downscale, bool d3d)
	{
		if (d3d)
		{ // downscale image as a whole
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "downscaling whole image");
			ILuint im = ilGenImage();
			ilBindImage(im);
			iluImageParameter(ILU_FILTER, ILU_BILINEAR);
			ilSetPixels(0, 0, 0, images[0].width, images[0].height, numeric_cast<uint32>(images.size()), convertBppToFormat(images[0].bpp), IL_UNSIGNED_BYTE, nullptr);
			for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
				it->saveToDevil(numeric_cast<uint32>(it - images.begin()));
			if (!iluScale(max(images[0].width / downscale, 1u), max(images[0].height / downscale, 1u), max(numeric_cast<uint32>(images.size()), 1u)))
				CAGE_THROW_ERROR(exception, "iluScale");
			for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
				it->loadFromDevil(numeric_cast<uint32>(it - images.begin()));
			ilBindImage(0);
			ilDeleteImage(im);
		}
		else
		{ // downscale each image separately
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "downscaling each slice separatelly");
			ILuint im = ilGenImage();
			ilBindImage(im);
			iluImageParameter(ILU_FILTER, ILU_BILINEAR);
			for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
			{
				CAGE_LOG(severityEnum::Info, logComponentName, string() + "downscaling slice: " + numeric_cast<uint32>(it - images.begin()));
				it->saveToDevil();
				if (!iluScale(max(it->width / downscale, 1u), max(it->height / downscale, 1u), 1))
					CAGE_THROW_ERROR(exception, "iluScale");
				it->loadFromDevil();
			}
			ilBindImage(0);
			ilDeleteImage(im);
		}
	}

	void performDegamma(float coef)
	{
		ILuint im = ilGenImage();
		ilBindImage(im);
		coef = 1.f / coef;
		for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
		{
			it->saveToDevil();
			if (!iluGammaCorrect(coef))
				CAGE_THROW_ERROR(exception, "iluGammaCorrect");
			it->loadFromDevil();
		}
		ilBindImage(0);
		ilDeleteImage(im);
	}

	void checkConsistency(uint32 target)
	{
		uint32 frames = numeric_cast<uint32>(images.size());
		if (frames == 0)
			CAGE_THROW_ERROR(exception, "no images were loaded");
		if (target == GL_TEXTURE_2D && frames > 1)
		{
			CAGE_LOG(severityEnum::Note, "exception", "did you forgot to set texture target?");
			CAGE_THROW_ERROR(exception, "images have too many frames for a 2D texture");
		}
		if (target == GL_TEXTURE_CUBE_MAP && frames != 6)
			CAGE_THROW_ERROR(exception, "cube texture requires exactly 6 images");
		if (target != GL_TEXTURE_2D && frames == 1)
			CAGE_LOG(severityEnum::Warning, logComponentName, "texture has only one frame. consider setting target to GL_TEXTURE_2D");
		imageLayerStruct &im0 = images[0];
		if (im0.width == 0 || im0.height == 0)
			CAGE_THROW_ERROR(exception, "image has zeroed resolution");
		if (im0.bpp == 0 || im0.bpp > 4)
			CAGE_THROW_ERROR(exception, "image has invalid bpp");
		for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
		{
			imageLayerStruct &imi = *it;
			if (imi.width != im0.width || imi.height != im0.height)
				CAGE_THROW_ERROR(exception, "frames has inconsistent resolutions");
			if (imi.bpp != im0.bpp)
				CAGE_THROW_ERROR(exception, "frames has inconsistent bpp");
		}
	}

	void exportTexture(uint32 target)
	{
#ifdef CAGE_DEBUG
		checkConsistency(target);
#endif

		textureHeaderStruct data;
		detail::memset(&data, 0, sizeof(textureHeaderStruct));
		data.target = target;
		data.filterMin = convertFilter(properties("filter_min"));
		data.filterMag = convertFilter(properties("filter_mag"));
		data.filterAniso = properties("filter_aniso").toUint32();
		data.wrapX = convertWrap(properties("wrap_x"));
		data.wrapY = convertWrap(properties("wrap_y"));
		data.wrapZ = convertWrap(properties("wrap_z"));
		data.flags =
			(requireMipmaps(data.filterMin) ? textureFlags::GenerateBitmap : textureFlags::None) |
			(properties("animation_loop").toBool() ? textureFlags::AnimationLoop : textureFlags::None);
		data.dimX = images[0].width;
		data.dimY = images[0].height;
		data.dimZ = numeric_cast<uint32>(images.size());
		data.bpp = images[0].bpp;
		switch (data.bpp)
		{
		case 1:
			data.copyFormat = GL_RED;
			data.internalFormat = GL_R8;
			break;
		case 2:
			data.copyFormat = GL_RG;
			data.internalFormat = GL_RG8;
			break;
		case 3:
			data.copyFormat = GL_RGB;
			data.internalFormat = GL_RGB8;
			break;
		case 4:
			data.copyFormat = GL_RGBA;
			data.internalFormat = GL_RGBA8;
			break;
		default:
			CAGE_THROW_ERROR(exception, "unsupported bpp");
		}
		data.copyType = GL_UNSIGNED_BYTE;
		data.stride = data.dimX * data.dimY * data.bpp;
		data.animationDuration = properties("animation_duration").toUint32();

		assetHeaderStruct h = initializeAssetHeaderStruct();
		h.originalSize = sizeof(data);
		for (auto it = images.begin(), et = images.end(); it != et; it++)
			h.originalSize += it->data.size();

		memoryBuffer inputBuffer(numeric_cast<uintPtr>(h.originalSize - sizeof(data)));
		{
			char *last = (char*)inputBuffer.data();
			for (auto it = images.begin(), et = images.end(); it != et; it++)
			{
				detail::memcpy(last, it->data.data(), it->data.size());
				last += it->data.size();
			}
		}
		memoryBuffer outputBuffer = detail::compress(inputBuffer);
		h.compressedSize = sizeof(data) + outputBuffer.size();
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "final size: " + h.originalSize + ", compressed size: " + h.compressedSize + ", ratio: " + h.compressedSize / (float)h.originalSize);

		holder<fileClass> f = newFile(outputFileName, fileMode(false, true));
		f->write(&h, sizeof(h));
		f->write(&data, sizeof(data));
		f->write(outputBuffer.data(), outputBuffer.size());
		f->close();
	}
}

void processTexture()
{
	if (properties("convert_normal").toFloat() > 0 && properties("premultiply_alpha").toBool())
		CAGE_THROW_ERROR(exception, "conversion to normal map and premultiplied alpha are incompatible");

	ilInit();
	iluInit();
	ilEnable(IL_CONV_PAL);
	ilEnable(IL_FILE_OVERWRITE);

	uint32 target = convertTarget(properties("target"));

	{ // load all files
		uint32 firstDollar = inputFile.find('$');
		if (firstDollar == -1)
			loadFile(inputFile);
		else
		{
			images.reserve(100);
			string prefix = inputFile.subString(0, firstDollar);
			string suffix = inputFile.subString(firstDollar, -1);
			uint32 dollarsCount = 0;
			while (!suffix.empty() && suffix[0] == '$')
			{
				dollarsCount++;
				suffix = suffix.subString(1, -1);
			}
			uint32 index = 0;
			while (true)
			{
				string name = prefix + string(index).reverse().fill(dollarsCount, '0').reverse() + suffix;
				if (!pathExists(pathJoin(inputDirectory, name)))
					break;
				CAGE_LOG(severityEnum::Info, logComponentName, string() + "loading file: '" + name + "'");
				loadFile(name);
				index++;
			}
		}
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "loading done");
	}

	checkConsistency(target);
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "resolution: " + images[0].width + "*" + images[0].height + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "channels: " + images[0].bpp);

	{ // convert height map to normal map
		float strength = properties("convert_normal").toFloat();
		if (strength > 0)
		{
			for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
				it->convertHeightToNormal(strength);
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "converted from height map to normal map with strength of " + strength);
		}
	}

	{ // premultiply alpha
		if (properties("premultiply_alpha").toBool())
		{
			for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
				it->premultiplyAlpha();
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "premultiplied alpha");
		}
	}

	{ // downscale
		uint32 downscale = properties("downscale").toUint32();
		if (downscale > 1)
		{
			performDownscale(downscale, target == GL_TEXTURE_3D);
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "downscaled: " + images[0].width + "*" + images[0].height + "*" + numeric_cast<uint32>(images.size()));
		}
	}

	if (properties("degamma_on").toBool())
	{ // degamma
		float coef = properties("degamma_coef").toFloat();
		performDegamma(coef);
		CAGE_LOG(severityEnum::Info, logComponentName, string() + "gamma correction by " + coef);
	}

	{ // vertical flip
		std::set<string> extsToSkip;
		extsToSkip.insert("tga");
		if (properties("flip_v").toBool() == (extsToSkip.find(pathExtractExtension(inputFile).toLower()) != extsToSkip.end()))
		{
			for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
				it->performFlipv();
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "image vertically flipped");
		}
	}

	exportTexture(target);

	if (configGetBool("cage-asset-processor.texture.preview"))
	{ // preview images
		for (std::vector<imageLayerStruct>::iterator it = images.begin(), et = images.end(); it != et; it++)
		{
			string dbgName = pathJoin(configGetString("cage-asset-processor.texture.path", "secondary-log"), pathMakeValid(inputName) + "_" + (it - images.begin()) + ".png");
			holder<pngImageClass> png = newPngImage();
			png->empty(it->width, it->height, it->bpp);
			detail::memcpy(png->bufferData(), it->data.data(), png->bufferSize());
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
