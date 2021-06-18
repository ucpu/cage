#include "image.h"

#include <cage-core/math.h>
#include <cage-core/color.h>
#include <cage-core/serialization.h>

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

	ImageColorConfig defaultConfig(uint32 channels)
	{
		ImageColorConfig c;
		switch (channels)
		{
		case 1:
			c.colorChannelsCount = 1;
			c.gammaSpace = GammaSpaceEnum::Gamma;
			break;
		case 2:
			c.colorChannelsCount = 1;
			c.gammaSpace = GammaSpaceEnum::Gamma;
			c.alphaChannelIndex = 1;
			c.alphaMode = AlphaModeEnum::Opacity;
			break;
		case 3:
			c.colorChannelsCount = 3;
			c.gammaSpace = GammaSpaceEnum::Gamma;
			break;
		case 4:
			c.colorChannelsCount = 3;
			c.gammaSpace = GammaSpaceEnum::Gamma;
			c.alphaChannelIndex = 3;
			c.alphaMode = AlphaModeEnum::Opacity;
			break;
		}
		return c;
	}

	void Image::clear()
	{
		ImageImpl *impl = (ImageImpl *)this;
		impl->width = 0;
		impl->height = 0;
		impl->channels = 0;
		impl->format = ImageFormatEnum::Default;
		impl->colorConfig = ImageColorConfig();
		impl->mem.resize(0);
	}

	void Image::initialize(uint32 w, uint32 h, uint32 c, ImageFormatEnum f)
	{
		CAGE_ASSERT(f != ImageFormatEnum::Default);
		CAGE_ASSERT(c > 0);
		CAGE_ASSERT(c == 3 || f != ImageFormatEnum::Rgbe);
		ImageImpl *impl = (ImageImpl *)this;
		impl->width = w;
		impl->height = h;
		impl->channels = c;
		impl->format = f;
		impl->mem.resize(0); // avoid unnecessary copies without deallocating the memory
		impl->mem.resize(w * h * c * formatBytes(f));
		impl->mem.zero();
		colorConfig = defaultConfig(c);
	}

	void Image::initialize(const ivec2 &r, uint32 c, ImageFormatEnum f)
	{
		CAGE_ASSERT(r[0] >= 0 && r[1] >= 0);
		initialize(r[0], r[0], c, f);
	}

	Holder<Image> Image::copy() const
	{
		const ImageImpl *src = (const ImageImpl *)this;
		Holder<Image> img = newImage();
		ImageImpl *dst = (ImageImpl *)+img;
		dst->mem = src->mem.copy();
		dst->width = src->width;
		dst->height = src->height;
		dst->channels = src->channels;
		dst->format = src->format;
		dst->colorConfig = src->colorConfig;
		return img;
	}

	void Image::importRaw(MemoryBuffer &&buffer, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format)
	{
		ImageImpl *impl = (ImageImpl *)this;
		CAGE_ASSERT(buffer.size() >= width * height * channels * formatBytes(format));
		initialize(0, 0, channels, format);
		impl->mem = std::move(buffer);
		impl->width = width;
		impl->height = height;
	}

	void Image::importRaw(MemoryBuffer &&buffer, const ivec2 &resolution, uint32 channels, ImageFormatEnum format)
	{
		CAGE_ASSERT(resolution[0] >= 0 && resolution[1] >= 0);
		importRaw(std::move(buffer), resolution[0], resolution[1], channels, format);
	}

	void Image::importRaw(PointerRange<const char> buffer, uint32 width, uint32 height, uint32 channels, ImageFormatEnum format)
	{
		ImageImpl *impl = (ImageImpl *)this;
		CAGE_ASSERT(buffer.size() >= width * height * channels * formatBytes(format));
		initialize(width, height, channels, format);
		detail::memcpy(impl->mem.data(), buffer.data(), impl->mem.size());
	}

	void Image::importRaw(PointerRange<const char> buffer, const ivec2 &resolution, uint32 channels, ImageFormatEnum format)
	{
		CAGE_ASSERT(resolution[0] >= 0 && resolution[1] >= 0);
		importRaw(buffer, resolution[0], resolution[1], channels, format);
	}

	uint32 Image::width() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		return impl->width;
	}

	uint32 Image::height() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		return impl->height;
	}

	ivec2 Image::resolution() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		return ivec2(impl->width, impl->height);
	}

	uint32 Image::channels() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		return impl->channels;
	}

	ImageFormatEnum Image::format() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		return impl->format;
	}

	float Image::value(uint32 x, uint32 y, uint32 c) const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
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

	float Image::value(const ivec2 &p, uint32 c) const
	{
		CAGE_ASSERT(p[0] >= 0 && p[1] >= 0);
		return value(p[0], p[1], c);
	}

	void Image::value(uint32 x, uint32 y, uint32 c, float v)
	{
		ImageImpl *impl = (ImageImpl *)this;
		CAGE_ASSERT(impl->channels == 3 || impl->format != ImageFormatEnum::Rgbe);
		CAGE_ASSERT(x < impl->width && y < impl->height && c < impl->channels);
		uint32 offset = (y * impl->width + x) * impl->channels;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
			((uint8 *)impl->mem.data())[offset + c] = numeric_cast<uint8>(saturate(v) * 255.f);
			break;
		case ImageFormatEnum::U16:
			((uint16 *)impl->mem.data())[offset + c] = numeric_cast<uint16>(saturate(v) * 65535.f);
			break;
		case ImageFormatEnum::Rgbe:
		{
			uint32 &p = ((uint32 *)impl->mem.data())[offset];
			vec3 s = colorRgbeToRgb(p);
			s[c] = saturate(v);
			p = colorRgbToRgbe(s);
		} break;
		case ImageFormatEnum::Float:
			((float *)impl->mem.data())[offset + c] = v;
			break;
		default:
			CAGE_THROW_CRITICAL(Exception, "invalid image format");
		}
	}

	void Image::value(const ivec2 &p, uint32 c, float v)
	{
		CAGE_ASSERT(p[0] >= 0 && p[1] >= 0);
		value(p[0], p[1], c, v);
	}

	void Image::value(uint32 x, uint32 y, uint32 c, const real &v)
	{
		value(x, y, c, v.value);
	}

	void Image::value(const ivec2 &p, uint32 c, const real &v)
	{
		CAGE_ASSERT(p[0] >= 0 && p[1] >= 0);
		value(p[0], p[1], c, v);
	}

	real Image::get1(uint32 x, uint32 y) const
	{
		CAGE_ASSERT(channels() == 1);
		return value(x, y, 0);
	}

	vec2 Image::get2(uint32 x, uint32 y) const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
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
		const ImageImpl *impl = (const ImageImpl *)this;
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
		const ImageImpl *impl = (const ImageImpl *)this;
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

	real Image::get1(const ivec2 &p) const { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); return get1(p[0], p[1]); }
	vec2 Image::get2(const ivec2 &p) const { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); return get2(p[0], p[1]); }
	vec3 Image::get3(const ivec2 &p) const { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); return get3(p[0], p[1]); }
	vec4 Image::get4(const ivec2 &p) const { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); return get4(p[0], p[1]); }

	void Image::get(uint32 x, uint32 y, real &value) const { value = get1(x, y); }
	void Image::get(uint32 x, uint32 y, vec2 &value) const { value = get2(x, y); }
	void Image::get(uint32 x, uint32 y, vec3 &value) const { value = get3(x, y); }
	void Image::get(uint32 x, uint32 y, vec4 &value) const { value = get4(x, y); }

	void Image::get(const ivec2 &p, real &value) const { value = get1(p); }
	void Image::get(const ivec2 &p, vec2 &value) const { value = get2(p); }
	void Image::get(const ivec2 &p, vec3 &value) const { value = get3(p); }
	void Image::get(const ivec2 &p, vec4 &value) const { value = get4(p); }

	void Image::set(uint32 x, uint32 y, const real &v)
	{
		CAGE_ASSERT(channels() == 1);
		value(x, y, 0, v.value);
	}

	void Image::set(uint32 x, uint32 y, const vec2 &v)
	{
		ImageImpl *impl = (ImageImpl *)this;
		CAGE_ASSERT(impl->channels == 2);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 2;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			vec2 vv = saturate(v) * 255;
			for (int i = 0; i < 2; i++)
				p[i] = numeric_cast<uint8>(vv[i]);
		} break;
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			vec2 vv = saturate(v) * 65535;
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
		ImageImpl *impl = (ImageImpl *)this;
		CAGE_ASSERT(impl->channels == 3);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 3;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			vec3 vv = saturate(v) * 255;
			for (int i = 0; i < 3; i++)
				p[i] = numeric_cast<uint8>(vv[i]);
		} break;
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			vec3 vv = saturate(v) * 65535;
			for (int i = 0; i < 3; i++)
				p[i] = numeric_cast<uint16>(vv[i]);
		} break;
		case ImageFormatEnum::Rgbe:
		{
			uint32 &p = ((uint32 *)impl->mem.data())[offset];
			p = colorRgbToRgbe(saturate(v));
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
		ImageImpl *impl = (ImageImpl *)this;
		CAGE_ASSERT(impl->channels == 4);
		CAGE_ASSERT(x < impl->width && y < impl->height);
		uint32 offset = (y * impl->width + x) * 4;
		switch (impl->format)
		{
		case ImageFormatEnum::U8:
		{
			uint8 *p = ((uint8 *)impl->mem.data()) + offset;
			vec4 vv = saturate(v) * 255;
			for (int i = 0; i < 4; i++)
				p[i] = numeric_cast<uint8>(vv[i]);
		} break;
		case ImageFormatEnum::U16:
		{
			uint16 *p = ((uint16 *)impl->mem.data()) + offset;
			vec4 vv = saturate(v) * 65535;
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

	void Image::set(const ivec2 &p, const real &value) { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); set(p[0], p[1], value); }
	void Image::set(const ivec2 &p, const vec2 &value) { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); set(p[0], p[1], value); }
	void Image::set(const ivec2 &p, const vec3 &value) { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); set(p[0], p[1], value); }
	void Image::set(const ivec2 &p, const vec4 &value) { CAGE_ASSERT(p[0] >= 0 && p[1] >= 0); set(p[0], p[1], value); }

	PointerRange<const uint8> Image::rawViewU8() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		CAGE_ASSERT(impl->format == ImageFormatEnum::U8);
		return bufferCast<const uint8, const char>(impl->mem);
	}

	PointerRange<const uint16> Image::rawViewU16() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		CAGE_ASSERT(impl->format == ImageFormatEnum::U16);
		return bufferCast<const uint16, const char>(impl->mem);
	}

	PointerRange<const float> Image::rawViewFloat() const
	{
		const ImageImpl *impl = (const ImageImpl *)this;
		CAGE_ASSERT(impl->format == ImageFormatEnum::Float);
		return bufferCast<const float, const char>(impl->mem);
	}

	Holder<Image> newImage()
	{
		return systemMemory().createImpl<Image, ImageImpl>();
	}
}
