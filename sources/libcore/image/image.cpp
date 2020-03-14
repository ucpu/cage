#include "image.h"

#include <cage-core/math.h>
#include <cage-core/color.h>

#include <utility>

namespace cage
{
	uint32 formatBytes(ImageFormatEnum format)
	{
		switch (format)
		{
		case ImageFormatEnum::U8: return sizeof(uint8);
		case ImageFormatEnum::U16: return sizeof(uint16);
		case ImageFormatEnum::Rgbe: return sizeof(uint32);
		case ImageFormatEnum::Float: return sizeof(float);
		default: CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	void Image::empty(uint32 w, uint32 h, uint32 c, ImageFormatEnum f)
	{
		CAGE_ASSERT(f != ImageFormatEnum::Default);
		CAGE_ASSERT(c > 0);
		CAGE_ASSERT(c == 3 || f != ImageFormatEnum::Rgbe);
		ImageImpl *impl = (ImageImpl*)this;
		reset();
		impl->width = w;
		impl->height = h;
		impl->channels = c;
		impl->format = f;
		impl->mem.resize(w * h * c * formatBytes(f));
		impl->mem.zero();
	}

	void Image::reset()
	{
		ImageImpl *impl = (ImageImpl*)this;
		impl->width = 0;
		impl->height = 0;
		impl->channels = 0;
		impl->format = ImageFormatEnum::Default;
		impl->colorConfig = ImageColorConfig();
		impl->mem.resize(0);
	}

	void Image::loadBuffer(const MemoryBuffer &buffer, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format)
	{
		loadMemory(buffer.data(), buffer.size(), width, height, channels, format);
	}

	void Image::loadMemory(const void *buffer, uintPtr size, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(size >= width * height * channels * formatBytes(format));
		empty(width, height, channels, format);
		detail::memcpy(impl->mem.data(), buffer, impl->mem.size());
	}

	uint32 Image::width() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		return impl->width;
	}

	uint32 Image::height() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		return impl->height;
	}

	uint32 Image::channels() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		return impl->channels;
	}

	ImageFormatEnum Image::format() const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		return impl->format;
	}

	float Image::value(uint32 x, uint32 y, uint32 c) const
	{
		const ImageImpl *impl = (const ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 3 || impl->format != ImageFormatEnum::Rgbe);
		CAGE_ASSERT(x < impl->width && y < impl->height && c < impl->channels);
		uint32 offset = (y * impl->width + x) * impl->channels;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
			return ((uint8 *)impl->mem.data())[offset + c] / 255.f;
		case ImageFormatEnum::U16:
			return ((uint16 *)impl->mem.data())[offset + c] / 65535.f;
		case ImageFormatEnum::Rgbe:
			return colorRgbeToRgb(((uint32 *)impl->mem.data())[offset])[c].value;
		case ImageFormatEnum::Float:
			return ((float *)impl->mem.data())[offset + c];
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	void Image::value(uint32 x, uint32 y, uint32 c, float v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 3 || impl->format != ImageFormatEnum::Rgbe);
		CAGE_ASSERT(x < impl->width && y < impl->height && c < impl->channels);
		uint32 offset = (y * impl->width + x) * impl->channels;
		v = clamp(v, 0.f, 1.f);
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
			((uint8 *)impl->mem.data())[offset + c] = numeric_cast<uint8>(v * 255.f);
			break;
		case ImageFormatEnum::U16:
			((uint16 *)impl->mem.data())[offset + c] = numeric_cast<uint16>(v * 65535.f);
			break;
		case ImageFormatEnum::Rgbe:
		{
			uint32 &p = ((uint32 *)impl->mem.data())[offset];
			vec3 s = colorRgbeToRgb(p);
			s[c] = v;
			p = colorRgbToRgbe(s);
		} break;
		case ImageFormatEnum::Float:
			((float *)impl->mem.data())[offset + c] = v;
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	void Image::value(uint32 x, uint32 y, uint32 c, const real &v) { value(x, y, c, v.value); }

	real Image::get1(uint32 x, uint32 y) const
	{
		CAGE_ASSERT(channels() == 1);
		return value(x, y, 0);
	}

	vec2 Image::get2(uint32 x, uint32 y) const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 2);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 2;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			return vec2(p[0], p[1]) / 255;
		}
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			return vec2(p[0], p[1]) / 65535;
		}
		case ImageFormatEnum::Float:
		{
			float *p = ((float *)impl->mem.data()) + offset;
			return vec2(p[0], p[1]);
		}
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	vec3 Image::get3(uint32 x, uint32 y) const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 3);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 3;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			return vec3(p[0], p[1], p[2]) / 255;
		}
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			return vec3(p[0], p[1], p[2]) / 65535;
		}
		case ImageFormatEnum::Rgbe:
		{
			uint32 p = ((uint32 *)impl->mem.data())[offset];
			return colorRgbeToRgb(p);
		}
		case ImageFormatEnum::Float:
		{
			float *p = ((float *)impl->mem.data()) + offset;
			return vec3(p[0], p[1], p[2]);
		}
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	vec4 Image::get4(uint32 x, uint32 y) const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 4);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 4;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			return vec4(p[0], p[1], p[2], p[3]) / 255;
		}
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			return vec4(p[0], p[1], p[2], p[3]) / 65535;
		}
		case ImageFormatEnum::Float:
		{
			float *p = ((float *)impl->mem.data()) + offset;
			return vec4(p[0], p[1], p[2], p[3]);
		}
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	void Image::get(uint32 x, uint32 y, real &value) const { value = get1(x, y); }
	void Image::get(uint32 x, uint32 y, vec2 &value) const { value = get2(x, y); }
	void Image::get(uint32 x, uint32 y, vec3 &value) const { value = get3(x, y); }
	void Image::get(uint32 x, uint32 y, vec4 &value) const { value = get4(x, y); }

	void Image::set(uint32 x, uint32 y, const real &v)
	{
		CAGE_ASSERT(channels() == 1);
		value(x, y, 0, v.value);
	}

	void Image::set(uint32 x, uint32 y, const vec2 &v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 2);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 2;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			vec2 vv = v * 255;
			for (int i = 0; i < 2; i++)
				p[i] = numeric_cast<uint8>(vv[i]);
		} break;
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			vec2 vv = v * 65535;
			for (int i = 0; i < 2; i++)
				p[i] = numeric_cast<uint16>(vv[i]);
		} break;
		case ImageFormatEnum::Float:
		{
			float *p = ((float *)impl->mem.data()) + offset;
			*(vec2 *)p = v;
		} break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	void Image::set(uint32 x, uint32 y, const vec3 &v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 3);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 3;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			vec3 vv = v * 255;
			for (int i = 0; i < 3; i++)
				p[i] = numeric_cast<uint8>(vv[i]);
		} break;
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			vec3 vv = v * 65535;
			for (int i = 0; i < 3; i++)
				p[i] = numeric_cast<uint16>(vv[i]);
		} break;
		case ImageFormatEnum::Rgbe:
		{
			uint32 &p = ((uint32 *)impl->mem.data())[offset];
			p = colorRgbToRgbe(v);
		} break;
		case ImageFormatEnum::Float:
		{
			float *p = ((float *)impl->mem.data()) + offset;
			*(vec3 *)p = v;
		} break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	void Image::set(uint32 x, uint32 y, const vec4 &v)
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->channels == 4);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 4;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			vec4 vv = v * 255;
			for (int i = 0; i < 4; i++)
				p[i] = numeric_cast<uint8>(vv[i]);
		} break;
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			vec4 vv = v * 65535;
			for (int i = 0; i < 4; i++)
				p[i] = numeric_cast<uint16>(vv[i]);
		} break;
		case ImageFormatEnum::Float:
		{
			float *p = ((float *)impl->mem.data()) + offset;
			*(vec4 *)p = v;
		} break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	PointerRange<const uint8> Image::rawViewU8() const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->format == ImageFormatEnum::U8);
		return { (uint8*)impl->mem.data(), (uint8*)(impl->mem.data() + impl->mem.size()) };
	}

	PointerRange<const uint16> Image::rawViewU16() const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->format == ImageFormatEnum::U16);
		return { (uint16*)impl->mem.data(), (uint16*)(impl->mem.data() + impl->mem.size()) };
	}

	PointerRange<const float> Image::rawViewFloat() const
	{
		ImageImpl *impl = (ImageImpl*)this;
		CAGE_ASSERT(impl->format == ImageFormatEnum::Float);
		return { (float*)impl->mem.data(), (float*)(impl->mem.data() + impl->mem.size()) };
	}

	Holder<Image> Image::copy() const
	{
		Holder<Image> img = newImage();
		imageBlit(this, img.get(), 0, 0, 0, 0, width(), height());
		return img;
	}

	void Image::verticalFlip()
	{
		ImageImpl *impl = (ImageImpl*)this;
		uint32 lineSize = formatBytes(impl->format) * impl->channels * impl->width;
		uint32 swapsCount = impl->height / 2;
		MemoryBuffer tmp;
		tmp.allocate(lineSize);
		for (uint32 i = 0; i < swapsCount; i++)
		{
			detail::memcpy(tmp.data(), impl->mem.data() + i * lineSize, lineSize);
			detail::memcpy(impl->mem.data() + i * lineSize, impl->mem.data() + (impl->height - i - 1) * lineSize, lineSize);
			detail::memcpy(impl->mem.data() + (impl->height - i - 1) * lineSize, tmp.data(), lineSize);
		}
	}

	namespace
	{
		void slice(const ImageImpl *source, ImageImpl *target)
		{
			uint32 copyChannels = min(source->channels, target->channels);
			for (uint32 y = 0; y < source->height; y++)
				for (uint32 x = 0; x < source->width; x++)
					for (uint32 c = 0; c < copyChannels; c++)
						target->value(x, y, c, source->value(x, y, c));
		}
	}

	void Image::convert(uint32 channels)
	{
		CAGE_ASSERT(channels > 0);
		ImageImpl *impl = (ImageImpl*)this;
		if (impl->channels == channels)
			return; // no op
		CAGE_ASSERT(impl->format != ImageFormatEnum::Rgbe);
		Holder<Image> tmp = newImage();
		tmp->empty(impl->width, impl->height, channels, impl->format);
		ImageImpl *t = (ImageImpl *)tmp.get();
		slice(impl, t);
		std::swap(impl->mem, t->mem);
		std::swap(impl->channels, t->channels);
	}

	void Image::convert(ImageFormatEnum format)
	{
		CAGE_ASSERT(format != ImageFormatEnum::Default);
		ImageImpl *impl = (ImageImpl*)this;
		if (impl->format == format)
			return; // no op
		Holder<Image> tmp = newImage();
		tmp->empty(impl->width, impl->height, impl->channels, format);
		imageBlit(this, tmp.get(), 0, 0, 0, 0, impl->width, impl->height);
		ImageImpl *t = (ImageImpl *)tmp.get();
		std::swap(impl->mem, t->mem);
		std::swap(impl->format, t->format);
	}

	void Image::convert(GammaSpaceEnum gammaSpace)
	{
		ImageImpl *impl = (ImageImpl*)this;
		if (gammaSpace == colorConfig.gammaSpace || colorConfig.colorChannelsCount == 0)
			return; // no op
		real p;
		if (gammaSpace == GammaSpaceEnum::Gamma && colorConfig.gammaSpace == GammaSpaceEnum::Linear)
			p = 1.0 / 2.2;
		else if (gammaSpace == GammaSpaceEnum::Linear && colorConfig.gammaSpace == GammaSpaceEnum::Gamma)
			p = 2.2;
		else
			CAGE_THROW_ERROR(Exception, "invalid image gamma conversion");
		const uint32 apply = min(impl->channels, colorConfig.colorChannelsCount);
		for (uint32 y = 0; y < impl->height; y++)
		{
			for (uint32 x = 0; x < impl->width; x++)
			{
				for (uint32 c = 0; c < apply; c++)
					value(x, y, c, pow(value(x, y, c), p));
			}
		}
		colorConfig.gammaSpace = gammaSpace;
	}

	void Image::convert(AlphaModeEnum alphaMode)
	{
		ImageImpl *impl = (ImageImpl*)this;
		if (alphaMode == colorConfig.alphaMode || colorConfig.colorChannelsCount == 0)
			return; // no op
		if (colorConfig.alphaChannelIndex >= impl->channels)
			CAGE_THROW_ERROR(Exception, "invalid alpha source channel index");
		if (colorConfig.alphaChannelIndex < colorConfig.colorChannelsCount)
			CAGE_THROW_ERROR(Exception, "alpha channel cannot overlap with color channels");
		const uint32 apply = colorConfig.colorChannelsCount;
		if (alphaMode == AlphaModeEnum::Opacity && colorConfig.alphaMode == AlphaModeEnum::PremultipliedOpacity)
		{
			for (uint32 y = 0; y < impl->height; y++)
			{
				for (uint32 x = 0; x < impl->width; x++)
				{
					real a = value(x, y, colorConfig.alphaChannelIndex);
					if (abs(a) < 1e-7)
					{
						for (uint32 c = 0; c < apply; c++)
							value(x, y, c, 0);
					}
					else
					{
						a = 1 / a;
						for (uint32 c = 0; c < apply; c++)
							value(x, y, c, value(x, y, c) * a);
					}
				}
			}
		}
		else if (alphaMode == AlphaModeEnum::PremultipliedOpacity && colorConfig.alphaMode == AlphaModeEnum::Opacity)
		{
			for (uint32 y = 0; y < impl->height; y++)
			{
				for (uint32 x = 0; x < impl->width; x++)
				{
					real a = value(x, y, colorConfig.alphaChannelIndex);
					for (uint32 c = 0; c < apply; c++)
						value(x, y, c, value(x, y, c) * a);
				}
			}
		}
		else
			CAGE_THROW_ERROR(Exception, "invalid image alpha conversion");
		colorConfig.alphaMode = alphaMode;
	}

	void Image::resize(uint32 w, uint32 h, bool useColorConfig)
	{
		ImageImpl *impl = (ImageImpl*)this;
		if (w == impl->width && h == impl->height)
			return; // no op

		ImageFormatEnum originalFormat = impl->format;
		ImageColorConfig originalColor = colorConfig;

		convert(ImageFormatEnum::Float);
		if (useColorConfig)
		{
			if (colorConfig.gammaSpace != GammaSpaceEnum::None)
				convert(GammaSpaceEnum::Linear);
			if (colorConfig.alphaMode != AlphaModeEnum::None)
				convert(AlphaModeEnum::PremultipliedOpacity);
		}

		{
			MemoryBuffer buff;
			buff.allocate(w * h * impl->channels * sizeof(float));
			detail::imageResize((float *)impl->mem.data(), impl->width, impl->height, 1, (float *)buff.data(), w, h, 1, impl->channels);
			std::swap(impl->mem, buff);
			impl->width = w;
			impl->height = h;
		}

		convert(originalColor.alphaMode);
		convert(originalColor.gammaSpace);
		convert(originalFormat);
	}

	Holder<Image> newImage()
	{
		return detail::systemArena().createImpl<Image, ImageImpl>();
	}

	namespace
	{
		bool overlaps(uint32 x1, uint32 y1, uint32 s)
		{
			if (x1 > y1)
				std::swap(x1, y1);
			uint32 x2 = x1 + s;
			uint32 y2 = y1 + s;
			return x1 < y2 && y1 < x2;
		}

		bool overlaps(uint32 x1, uint32 y1, uint32 x2, uint32 y2, uint32 w, uint32 h)
		{
			return overlaps(x1, x2, w) && overlaps(y1, y2, h);
		}
	}

	void imageBlit(const Image *sourceImg, Image *targetImg, uint32 sourceX, uint32 sourceY, uint32 targetX, uint32 targetY, uint32 width, uint32 height)
	{
		ImageImpl *s = (ImageImpl*)sourceImg;
		ImageImpl *t = (ImageImpl*)targetImg;
		CAGE_ASSERT(s->format != ImageFormatEnum::Default && s->channels > 0);
		CAGE_ASSERT(s != t || !overlaps(sourceX, sourceY, targetX, targetY, width, height));
		if (t->format == ImageFormatEnum::Default && targetX == 0 && targetY == 0)
		{
			t->empty(width, height, s->channels, s->format);
			t->colorConfig = s->colorConfig;
		}
		CAGE_ASSERT(s->channels == t->channels);
		CAGE_ASSERT(sourceX + width <= s->width);
		CAGE_ASSERT(sourceY + height <= s->height);
		CAGE_ASSERT(targetX + width <= t->width);
		CAGE_ASSERT(targetY + height <= t->height);
		if (s->format == t->format)
		{
			uint32 ps = formatBytes(s->format) * s->channels;
			uint32 sl = s->width * ps;
			uint32 tl = t->width * ps;
			char *ss = s->mem.data();
			char *tt = t->mem.data();
			for (uint32 y = 0; y < height; y++)
				detail::memcpy(tt + (targetY + y) * tl + targetX * ps, ss + (sourceY + y) * sl + sourceX * ps, width * ps);
		}
		else
		{
			uint32 cc = s->channels;
			switch (cc)
			{
			case 1:
			{
				for (uint32 y = 0; y < height; y++)
					for (uint32 x = 0; x < width; x++)
						t->set(targetX + x, targetY + y, s->get1(sourceX + x, sourceY + y));
			} break;
			case 2:
			{
				for (uint32 y = 0; y < height; y++)
					for (uint32 x = 0; x < width; x++)
						t->set(targetX + x, targetY + y, s->get2(sourceX + x, sourceY + y));
			} break;
			case 3:
			{
				for (uint32 y = 0; y < height; y++)
					for (uint32 x = 0; x < width; x++)
						t->set(targetX + x, targetY + y, s->get3(sourceX + x, sourceY + y));
			} break;
			case 4:
			{
				for (uint32 y = 0; y < height; y++)
					for (uint32 x = 0; x < width; x++)
						t->set(targetX + x, targetY + y, s->get4(sourceX + x, sourceY + y));
			} break;
			default:
			{
				for (uint32 y = 0; y < height; y++)
					for (uint32 x = 0; x < width; x++)
						for (uint32 c = 0; c < cc; c++)
							t->value(targetX + x, targetY + y, c, s->value(sourceX + x, sourceY + y, c));
			} break;
			}
		}
	}
}
