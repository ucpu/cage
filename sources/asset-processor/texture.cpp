#include <vector>
#include <set>

#include "processor.h"

#include <cage-core/memoryBuffer.h>
#include <cage-core/png.h>
#include <cage-core/color.h>

#include <cage-client/opengl.h>

#include <IL/il.h>
#include <IL/ilu.h>

vec2 convertSpecularToSpecial(const vec3 &spec)
{
	vec3 hsv = convertRgbToHsv(spec);
	return vec2(1 - hsv[2], hsv[1]);
	/*
	real r = (spec[0] + spec[1] + spec[2]) / 3;
	vec3 d = abs(spec - r);
	real m = (d[0] + d[1] + d[2]) / 3;
	CAGE_ASSERT_RUNTIME(r >= 0 && r <= 1, r, m, spec);
	CAGE_ASSERT_RUNTIME(m >= 0 && m <= 1, r, m, spec);
	return vec2(1 - r, m);
	*/
}

namespace
{
	uint32 convertFilter(const string &f)
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

	uint32 convertTarget(const string &f)
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

	ILenum convertBppToFormat(uint32 bpp)
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
					v = v.normalize();
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
					CAGE_ASSERT_RUNTIME(special[1] < 1e-7);
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
				CAGE_THROW_ERROR(exception, "1 or 3 channels are required for conversion of specular color to special material");
			}
		}

		void invert(uint32 channelIndex)
		{
			if (channelIndex >= bpp)
				CAGE_THROW_ERROR(exception, "texture does not have that channel");
			uint32 pixels = width * height;
			for (uint32 i = 0; i < pixels; i++)
				data[i * bpp + channelIndex] = 255 - data[i * bpp + channelIndex];
		}

		void premultiplyAlpha()
		{
			if (bpp != 4)
				CAGE_THROW_ERROR(exception, "premultiplied alpha requires 4 components");
			if (properties("srgb").toBool())
				premultiplyAlphaImpl<true>();
			else
				premultiplyAlphaImpl<false>();
		}

	private:

		template<bool SRGB>
		void premultiplyAlphaImpl()
		{
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
				CAGE_THROW_CRITICAL(exception, "invalid bpp");
			}
			return sum / 255.f / bpp;
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
			images[0].resizeDevil(numeric_cast<uint32>(images.size()));
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
			images[0].resizeDevil();
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
		for (auto &imi : images)
		{
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
				CAGE_THROW_ERROR(exception, "unsupported bpp");
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
				CAGE_THROW_ERROR(exception, "unsupported bpp");
			}
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
			char *last = inputBuffer.data();
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
	{
		bool n = properties("convert_normal").toFloat() > 0;
		bool s = properties("convert_specular_to_special").toBool();
		bool a = properties("premultiply_alpha").toBool();
		bool g = properties("srgb").toBool();
		//bool inv_r = properties("invert_r").toBool();
		//bool inv_g = properties("invert_g").toBool();
		//bool inv_b = properties("invert_b").toBool();
		//bool inv_a = properties("invert_a").toBool();
		if (n && s)
			CAGE_THROW_ERROR(exception, "only one conversion is possible");
		if ((n || s) && a)
			CAGE_THROW_ERROR(exception, "premultiplied alpha is only for colors");
		if ((n || s) && g)
			CAGE_THROW_ERROR(exception, "srgb is only for colors");
	}

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

	CAGE_LOG(severityEnum::Info, logComponentName, string() + "resolution: " + images[0].width + "*" + images[0].height + "*" + numeric_cast<uint32>(images.size()));
	CAGE_LOG(severityEnum::Info, logComponentName, string() + "channels: " + images[0].bpp);
	checkConsistency(target);

	{ // convert height map to normal map
		float strength = properties("convert_normal").toFloat();
		if (strength > 0)
		{
			for (auto &it : images)
				it.convertHeightToNormal(strength);
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "converted from height map to normal map with strength of " + strength);
		}
	}

	{ // convert specular to special
		if (properties("convert_specular_to_special").toBool())
		{
			for (auto &it : images)
				it.convertSpecularToSpecial();
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "converted from specular colors to material special");
		}
	}

	{ // premultiply alpha
		if (properties("premultiply_alpha").toBool())
		{
			for (auto &it : images)
				it.premultiplyAlpha();
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

	{ // vertical flip
		if (!properties("flip_v").toBool())
		{
			for (auto &it : images)
				it.performFlipV();
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "image vertically flipped (flip_v was false)");
		}
	}

	{ // invert
		if (properties("invert_r").toBool())
		{
			for (auto &it : images)
				it.invert(0);
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "red channel inverted");
		}
		if (properties("invert_g").toBool())
		{
			for (auto &it : images)
				it.invert(1);
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "green channel inverted");
		}
		if (properties("invert_b").toBool())
		{
			for (auto &it : images)
				it.invert(2);
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "blue channel inverted");
		}
		if (properties("invert_a").toBool())
		{
			for (auto &it : images)
				it.invert(3);
			CAGE_LOG(severityEnum::Info, logComponentName, string() + "alpha channel inverted");
		}
	}

	exportTexture(target);

	if (configGetBool("cage-asset-processor.texture.preview"))
	{ // preview images
		uint32 index = 0;
		for (auto &it : images)
		{
			string dbgName = pathJoin(configGetString("cage-asset-processor.texture.path", "asset-preview"), pathMakeValid(inputName) + "_" + (index++) + ".png");
			holder<pngImageClass> png = newPngImage();
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
