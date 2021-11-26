#include "image.h"

#include <cage-core/math.h>
#include <cage-core/color.h>

namespace cage
{
	void imageFill(Image *img, const Real &value)
	{
		ImageImpl *impl = (ImageImpl*)img;
		for (uint32 y = 0; y < impl->height; y++)
			for (uint32 x = 0; x < impl->width; x++)
				impl->set(x, y, value);
	}

	void imageFill(Image *img, const Vec2 &value)
	{
		ImageImpl *impl = (ImageImpl*)img;
		for (uint32 y = 0; y < impl->height; y++)
			for (uint32 x = 0; x < impl->width; x++)
				impl->set(x, y, value);
	}

	void imageFill(Image *img, const Vec3 &value)
	{
		ImageImpl *impl = (ImageImpl*)img;
		for (uint32 y = 0; y < impl->height; y++)
			for (uint32 x = 0; x < impl->width; x++)
				impl->set(x, y, value);
	}

	void imageFill(Image *img, const Vec4 &value)
	{
		ImageImpl *impl = (ImageImpl*)img;
		for (uint32 y = 0; y < impl->height; y++)
			for (uint32 x = 0; x < impl->width; x++)
				impl->set(x, y, value);
	}

	void imageFill(Image *img, uint32 ch, float val)
	{
		ImageImpl *impl = (ImageImpl*)img;
		for (uint32 y = 0; y < impl->height; y++)
			for (uint32 x = 0; x < impl->width; x++)
				impl->value(x, y, ch, val);
	}

	void imageFill(Image *img, uint32 ch, const Real &val)
	{
		ImageImpl *impl = (ImageImpl*)img;
		for (uint32 y = 0; y < impl->height; y++)
			for (uint32 x = 0; x < impl->width; x++)
				impl->value(x, y, ch, val);
	}

	void imageVerticalFlip(Image *img)
	{
		ImageImpl *impl = (ImageImpl*)img;
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
			const uint32 copyChannels = min(source->channels, target->channels);
			for (uint32 y = 0; y < source->height; y++)
				for (uint32 x = 0; x < source->width; x++)
					for (uint32 c = 0; c < copyChannels; c++)
						target->value(x, y, c, source->value(x, y, c));
		}
	}

	void imageConvert(Image *img, uint32 channels)
	{
		CAGE_ASSERT(channels > 0);
		ImageImpl *impl = (ImageImpl*)img;
		if (impl->channels == channels)
			return; // no op
		CAGE_ASSERT(impl->format != ImageFormatEnum::Rgbe);
		Holder<Image> tmp = newImage();
		tmp->initialize(impl->width, impl->height, channels, impl->format);
		ImageImpl *t = (ImageImpl *)tmp.get();
		slice(impl, t);
		std::swap(impl->mem, t->mem);
		std::swap(impl->channels, t->channels);
	}

	void imageConvert(Image *img, ImageFormatEnum format)
	{
		CAGE_ASSERT(format != ImageFormatEnum::Default);
		ImageImpl *impl = (ImageImpl*)img;
		if (impl->format == format)
			return; // no op
		Holder<Image> tmp = newImage();
		tmp->initialize(impl->width, impl->height, impl->channels, format);
		imageBlit(img, +tmp, 0, 0, 0, 0, impl->width, impl->height);
		ImageImpl *t = (ImageImpl *)+tmp;
		std::swap(impl->mem, t->mem);
		std::swap(impl->format, t->format);
	}

	void imageConvert(Image *img, GammaSpaceEnum gammaSpace)
	{
		ImageImpl *impl = (ImageImpl*)img;
		if (gammaSpace == impl->colorConfig.gammaSpace || impl->colorConfig.colorChannelsCount == 0)
			return; // no op
		Real p;
		if (gammaSpace == GammaSpaceEnum::Gamma && impl->colorConfig.gammaSpace == GammaSpaceEnum::Linear)
			p = 1.0 / 2.2;
		else if (gammaSpace == GammaSpaceEnum::Linear && impl->colorConfig.gammaSpace == GammaSpaceEnum::Gamma)
			p = 2.2;
		else
			CAGE_THROW_ERROR(Exception, "invalid image gamma conversion");
		const uint32 apply = min(impl->channels, impl->colorConfig.colorChannelsCount);
		for (uint32 y = 0; y < impl->height; y++)
		{
			for (uint32 x = 0; x < impl->width; x++)
			{
				for (uint32 c = 0; c < apply; c++)
					impl->value(x, y, c, pow(impl->value(x, y, c), p));
			}
		}
		impl->colorConfig.gammaSpace = gammaSpace;
	}

	void imageConvert(Image *img, AlphaModeEnum alphaMode)
	{
		ImageImpl *impl = (ImageImpl*)img;
		if (alphaMode == impl->colorConfig.alphaMode || impl->colorConfig.colorChannelsCount == 0)
			return; // no op
		if (impl->colorConfig.alphaChannelIndex >= impl->channels)
			CAGE_THROW_ERROR(Exception, "invalid alpha source channel index");
		if (impl->colorConfig.alphaChannelIndex < impl->colorConfig.colorChannelsCount)
			CAGE_THROW_ERROR(Exception, "alpha channel cannot overlap with color channels");
		const uint32 apply = impl->colorConfig.colorChannelsCount;
		if (alphaMode == AlphaModeEnum::Opacity && impl->colorConfig.alphaMode == AlphaModeEnum::PremultipliedOpacity)
		{
			for (uint32 y = 0; y < impl->height; y++)
			{
				for (uint32 x = 0; x < impl->width; x++)
				{
					Real a = impl->value(x, y, impl->colorConfig.alphaChannelIndex);
					if (abs(a) < 1e-7)
					{
						for (uint32 c = 0; c < apply; c++)
							impl->value(x, y, c, 0);
					}
					else
					{
						a = 1 / a;
						for (uint32 c = 0; c < apply; c++)
							impl->value(x, y, c, impl->value(x, y, c) * a);
					}
				}
			}
		}
		else if (alphaMode == AlphaModeEnum::PremultipliedOpacity && impl->colorConfig.alphaMode == AlphaModeEnum::Opacity)
		{
			for (uint32 y = 0; y < impl->height; y++)
			{
				for (uint32 x = 0; x < impl->width; x++)
				{
					Real a = impl->value(x, y, impl->colorConfig.alphaChannelIndex);
					for (uint32 c = 0; c < apply; c++)
						impl->value(x, y, c, impl->value(x, y, c) * a);
				}
			}
		}
		else
			CAGE_THROW_ERROR(Exception, "invalid image alpha conversion");
		impl->colorConfig.alphaMode = alphaMode;
	}

	namespace
	{
		Real convertToNormalIntensity(const Image *img, sint32 x, sint32 y)
		{
			x = min((sint32)img->width() - 1, max(0, x));
			y = min((sint32)img->height() - 1, max(0, y));
			Real sum = 0;
			const uint32 c = img->channels();
			for (uint32 i = 0; i < c; i++)
				sum += img->value(x, y, i);
			return sum / img->channels();
		}
	}

	void imageConvertHeigthToNormal(Image *img, const Real &strength_)
	{
		const Real strength = 1.f / strength_;
		const uint32 w = img->width();
		const uint32 h = img->height();
		Holder<Image> src = img->copy();
		img->initialize(w, h, 3, src->format());
		for (sint32 y = 0; (uint32)y < h; y++)
		{
			for (sint32 x = 0; (uint32)x < w; x++)
			{
				const Real tl = convertToNormalIntensity(+src, x - 1, y - 1);
				const Real tc = convertToNormalIntensity(+src, x + 0, y - 1);
				const Real tr = convertToNormalIntensity(+src, x + 1, y - 1);
				const Real rc = convertToNormalIntensity(+src, x + 1, y + 0);
				const Real br = convertToNormalIntensity(+src, x + 1, y + 1);
				const Real bc = convertToNormalIntensity(+src, x + 0, y + 1);
				const Real bl = convertToNormalIntensity(+src, x - 1, y + 1);
				const Real lc = convertToNormalIntensity(+src, x - 1, y + 0);
				const Real dX = (tr + 2.f * rc + br) - (tl + 2.f * lc + bl);
				const Real dY = (bl + 2.f * bc + br) - (tl + 2.f * tc + tr);
				Vec3 v(-dX, -dY, strength);
				v = normalize(v);
				v += 1;
				v *= 0.5;
				img->set(x, y, v);
			}
		}
	}

	void imageConvertSpecularToSpecial(Image *img)
	{
		const uint32 w = img->width();
		const uint32 h = img->height();
		switch (img->channels())
		{
		case 1:
		{
			for (uint32 y = 0; y < h; y++)
			{
				for (uint32 x = 0; x < w; x++)
				{
					const Vec3 color = Vec3(img->get1(x, y));
					const Vec2 special = colorSpecularToRoughnessMetallic(color);
					CAGE_ASSERT(special[1] < 1e-7);
					img->set(x, y, special[0]);
				}
			}
		} break;
		case 3:
		{
			Holder<Image> src = img->copy();
			img->initialize(w, h, 2, src->format());
			for (uint32 y = 0; y < h; y++)
			{
				for (uint32 x = 0; x < w; x++)
				{
					Vec3 color = src->get3(x, y);
					Vec2 special = colorSpecularToRoughnessMetallic(color);
					img->set(x, y, special);
				}
			}
		} break;
		default:
			CAGE_THROW_ERROR(Exception, "exactly 1 or 3 channels are required for conversion of specular color to special material");
		}
	}

	void imageResize(Image *img, uint32 w, uint32 h, bool useColorConfig)
	{
		ImageImpl *impl = (ImageImpl*)img;
		if (w == impl->width && h == impl->height)
			return; // no op

		ImageFormatEnum originalFormat = impl->format;
		ImageColorConfig originalColor = impl->colorConfig;

		imageConvert(impl, ImageFormatEnum::Float);
		if (useColorConfig)
		{
			if (impl->colorConfig.gammaSpace != GammaSpaceEnum::None)
				imageConvert(impl, GammaSpaceEnum::Linear);
			if (impl->colorConfig.alphaMode != AlphaModeEnum::None)
				imageConvert(impl, AlphaModeEnum::PremultipliedOpacity);
		}

		{
			MemoryBuffer buff;
			buff.allocate(w * h * impl->channels * sizeof(float));
			detail::imageResize((float *)impl->mem.data(), impl->width, impl->height, 1, (float *)buff.data(), w, h, 1, impl->channels);
			std::swap(impl->mem, buff);
			impl->width = w;
			impl->height = h;
		}

		imageConvert(impl, originalColor.alphaMode);
		imageConvert(impl, originalColor.gammaSpace);
		imageConvert(impl, originalFormat);
	}

	void imageResize(Image *img, const Vec2i &r, bool useColorConfig)
	{
		CAGE_ASSERT(r[0] >= 0 && r[1] >= 0);
		imageResize(img, r[0], r[1], useColorConfig);
	}

	void imageBoxBlur(Image *img, uint32 radius, uint32 rounds, bool useColorConfig)
	{
		CAGE_THROW_CRITICAL(NotImplemented, "imageBoxBlur");
	}

	namespace
	{
		template<class T, bool UseNan>
		struct Dilation
		{
			Image *const src;
			Image *const dst;
			const uint32 w;
			const uint32 h;

			Dilation(Image *src, Image *dst) : src(src), dst(dst), w(src->width()), h(src->height())
			{}

			CAGE_FORCE_INLINE bool valid(const T &v)
			{
				if (UseNan)
					return cage::valid(v);
				return v != T();
			}

			CAGE_FORCE_INLINE void update(const T &a, T &m, uint32 &cnt)
			{
				const bool v = valid(a);
				if (UseNan)
				{
					if (v)
					{
						m += a;
						cnt++;
					}
				}
				else
				{
					m += a * v;
					cnt += v;
				}
			}

			CAGE_FORCE_INLINE void processPixelSimple(uint32 x, uint32 y)
			{
				T m;
				src->get(x, y, m);
				if (valid(m))
					dst->set(x, y, m);
				else
				{
					m = T();
					uint32 cnt = 0;
					const uint32 sy = numeric_cast<uint32>(clamp(sint32(y) - 1, 0, sint32(h) - 1));
					const uint32 ey = numeric_cast<uint32>(clamp(sint32(y) + 1, 0, sint32(h) - 1));
					const uint32 sx = numeric_cast<uint32>(clamp(sint32(x) - 1, 0, sint32(w) - 1));
					const uint32 ex = numeric_cast<uint32>(clamp(sint32(x) + 1, 0, sint32(w) - 1));
					for (uint32 yy = sy; yy <= ey; yy++)
					{
						for (uint32 xx = sx; xx <= ex; xx++)
						{
							T a;
							src->get(xx, yy, a);
							update(a, m, cnt);
						}
					}
					if (cnt > 0)
						dst->set(x, y, m / cnt);
				}
			}

			void processRowSimple(uint32 y)
			{
				for (uint32 x = 0; x < w; x++)
					processPixelSimple(x, y);
			}

			void processRowCenter(uint32 y)
			{
				const T *base = (T *)((ImageImpl *)src)->mem.data();
				const T *topRow = base + (y - 1) * w;
				const T *centerRow = base + y * w;
				const T *bottomRow = base + (y + 1) * w;

				T *outputBase = (T *)((ImageImpl *)dst)->mem.data();
				T *outputRow = outputBase + y * w;

				for (uint32 x = 1; x < w - 1; x++)
				{
					T m = centerRow[x];
					if (valid(m))
						outputRow[x] = m;
					else
					{
						m = T();
						uint32 cnt = 0;
						for (const T *row : { topRow, centerRow, bottomRow })
						{
							const T *r = row + x - 1;
							for (uint32 xx = 0; xx < 3; xx++)
								update(*r++, m, cnt);
						}
						if (cnt > 0)
							outputRow[x] = m / cnt;
					}
				}
			}

			void process()
			{
				if (w < 3 || h < 3)
				{
					for (uint32 y = 0; y < h; y++)
						processRowSimple(y);
					return;
				}
				processRowSimple(0);
				processRowSimple(h - 1);
				for (uint32 y = 1; y < h - 1; y++)
				{
					processPixelSimple(0, y);
					processRowCenter(y);
					processPixelSimple(w - 1, y);
				}
			}
		};

		template<class T, bool UseNan>
		void dilationProcess(Image *img, uint32 rounds)
		{
			CAGE_ASSERT(img);
			CAGE_ASSERT(img->format() == ImageFormatEnum::Float);

			Holder<Image> tmp = newImage();
			tmp->initialize(img->width(), img->height(), img->channels(), img->format());
			if (UseNan)
				imageFill(+tmp, T::Nan());

			Image *src = img;
			Image *dst = +tmp;

			for (uint32 r = 0; r < rounds; r++)
			{
				Dilation<T, UseNan> dilation(src, dst);
				dilation.process();
				std::swap(src, dst);
			}

			if (src != img)
				imageBlit(src, img, 0, 0, 0, 0, img->width(), img->height());
		}

		template<class T>
		void dilationProcess(Image *img, uint32 rounds, bool useNan)
		{
			if (useNan)
				dilationProcess<T, true>(img, rounds);
			else
				dilationProcess<T, false>(img, rounds);
		}
	}

	void imageDilation(Image *img, uint32 rounds, bool useNan)
	{
		const ImageFormatEnum originalFormat = img->format();
		imageConvert(img, ImageFormatEnum::Float);
		switch (img->channels())
		{
		case 1: dilationProcess<Real>(img, rounds, useNan); break;
		case 2: dilationProcess<Vec2>(img, rounds, useNan); break;
		case 3: dilationProcess<Vec3>(img, rounds, useNan); break;
		case 4: dilationProcess<Vec4>(img, rounds, useNan); break;
		default: CAGE_THROW_CRITICAL(NotImplemented, "image dilation with more than 4 channels");
		}
		imageConvert(img, originalFormat);
	}

	void imageInvertColors(Image *img, bool useColorConfig)
	{
		const uint32 w = img->width();
		const uint32 h = img->height();
		const uint32 c = useColorConfig ? img->colorConfig.colorChannelsCount : img->channels();
		for (uint32 y = 0; y < h; y++)
			for (uint32 x = 0; x < w; x++)
				for (uint32 i = 0; i < c; i++)
					img->value(x, y, i, 1 - img->value(x, y, i));
	}

	void imageInvertChannel(Image *img, uint32 channelIndex)
	{
		if (channelIndex >= img->channels())
			CAGE_THROW_ERROR(Exception, "image does not have selected channel");
		const uint32 w = img->width();
		const uint32 h = img->height();
		for (uint32 y = 0; y < h; y++)
		{
			for (uint32 x = 0; x < w; x++)
				img->value(x, y, channelIndex, 1 - img->value(x, y, channelIndex));
		}
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
		const ImageImpl *s = (const ImageImpl *)sourceImg;
		ImageImpl *t = (ImageImpl *)targetImg;
		CAGE_ASSERT(s->format != ImageFormatEnum::Default && s->channels > 0);
		CAGE_ASSERT(s != t || !overlaps(sourceX, sourceY, targetX, targetY, width, height));
		if (t->format == ImageFormatEnum::Default && targetX == 0 && targetY == 0)
		{
			t->initialize(width, height, s->channels, s->format);
			t->colorConfig = s->colorConfig;
		}
		CAGE_ASSERT(s->channels == t->channels);
		CAGE_ASSERT(sourceX + width <= s->width);
		CAGE_ASSERT(sourceY + height <= s->height);
		CAGE_ASSERT(targetX + width <= t->width);
		CAGE_ASSERT(targetY + height <= t->height);
		if (s->format == t->format)
		{
			const uint32 ps = formatBytes(s->format) * s->channels;
			const uint32 sl = s->width * ps;
			const uint32 tl = t->width * ps;
			const char *ss = s->mem.data();
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

	void imageBlit(const Image *source, Image *target, const Vec2i &sourceOffset, const Vec2i &targetOffset, const Vec2i &resolution)
	{
		imageBlit(source, target, sourceOffset[0], sourceOffset[1], targetOffset[0], targetOffset[1], resolution[0], resolution[1]);
	}
}
