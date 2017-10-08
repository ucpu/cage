#include "lodepng/lodepng.h"

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/filesystem.h>
#include <cage-core/utility/memoryBuffer.h>
#include <cage-core/utility/png.h>

namespace cage
{
	namespace
	{
		LodePNGColorType convertModeToLode(uint32 mode)
		{
			switch (mode)
			{
			case 1: return LCT_GREY;
			case 2: return LCT_GREY_ALPHA;
			case 3: return LCT_RGB;
			case 4: return LCT_RGBA;
			default: CAGE_THROW_ERROR(exception, "invalid png mode");
			}
		}

		class pngBufferImpl : public pngImageClass
		{
		public:
			memoryBuffer mem;
			uint32 width, height, channels, bytesPerChannel;

			pngBufferImpl() : width(0), height(0), channels(0), bytesPerChannel(0)
			{}

			void endianity()
			{
				if (privat::endianness::little())
				{
					switch (bytesPerChannel)
					{
					case 1:
						return;
					case 2:
					{
						uint16 *m = (uint16*)mem.data();
						CAGE_ASSERT_RUNTIME(mem.size() % 2 == 0, mem.size());
						uint32 cnt = numeric_cast<uint32>(mem.size() / 2);
						for (uint32 i = 0; i < cnt; i++, m++)
							*m = privat::endianness::change(*m);
					} break;
					default: CAGE_THROW_ERROR(exception, "invalid png bpc");
					}
				}
			}
		};
	}

	void pngImageClass::empty(uint32 w, uint32 h, uint32 c, uint32 bpc)
	{
		pngBufferImpl *impl = (pngBufferImpl*)this;
		impl->width = w;
		impl->height = h;
		impl->channels = c;
		impl->bytesPerChannel = bpc;
		impl->mem.reallocate(w * h * c * bpc);
		impl->mem.clear();
	}

	void pngImageClass::encodeBuffer(memoryBuffer &buffer)
	{
		pngBufferImpl *impl = (pngBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->mem.data(), "png image not initialized");
		void *out = nullptr;
		try
		{
			size_t size = 0;
			unsigned res = lodepng_encode_memory((unsigned char **)&out, &size, (const unsigned char *)impl->mem.data(), impl->width, impl->height, convertModeToLode(impl->channels), impl->bytesPerChannel * 8);
			if (res != 0)
			{
				CAGE_LOG(severityEnum::Note, "exception", lodepng_error_text(res));
				CAGE_THROW_ERROR(exception, "decode png");
			}
			buffer.reallocate(size);
			detail::memcpy(buffer.data(), out, size);
			impl->endianity();
		}
		catch (...)
		{
			free(out);
			throw;
		}
		free(out);
	}

	void pngImageClass::encodeFile(const string &filename)
	{
		memoryBuffer buffer;
		encodeBuffer(buffer);
		holder<fileClass> f = newFile(filename, fileMode(false, true));
		f->write(buffer.data(), buffer.size());
	}

	void pngImageClass::decodeMemory(const void *buffer, uintPtr size)
	{
		unsigned w = 0, h = 0;
		LodePNGState state;
		lodepng_state_init(&state);
		unsigned res = lodepng_inspect(&w, &h, &state, (const unsigned char *)buffer, size);
		if (res != 0)
		{
			CAGE_LOG(severityEnum::Note, "exception", lodepng_error_text(res));
			CAGE_THROW_ERROR(exception, "decode png");
		}
		decodeMemory(buffer, size, lodepng_get_channels(&state.info_png.color), state.info_png.color.bitdepth / 8);
		lodepng_state_cleanup(&state);
	}

	void pngImageClass::decodeMemory(const void *buffer, uintPtr size, uint32 channels, uint32 bpc)
	{
		pngBufferImpl *impl = (pngBufferImpl*)this;
		void *out = nullptr;
		try
		{
			impl->mem.free();
			impl->channels = channels;
			impl->bytesPerChannel = bpc;
			unsigned res = lodepng_decode_memory((unsigned char **)&out, &impl->width, &impl->height, (const unsigned char *)buffer, size, convertModeToLode(channels), bpc * 8);
			if (res != 0)
			{
				CAGE_LOG(severityEnum::Note, "exception", lodepng_error_text(res));
				CAGE_THROW_ERROR(exception, "decode png");
			}
			impl->mem.reallocate(impl->width * impl->height * channels * bpc);
			detail::memcpy(impl->mem.data(), out, impl->mem.size());
			impl->endianity();
		}
		catch (...)
		{
			free(out);
			throw;
		}
		free(out);
	}

	void pngImageClass::decodeBuffer(const memoryBuffer &buffer)
	{
		decodeMemory(buffer.data(), buffer.size());
	}

	void pngImageClass::decodeBuffer(const struct memoryBuffer &buffer, uint32 channels, uint32 bpc)
	{
		decodeMemory(buffer.data(), buffer.size(), channels, bpc);
	}

	void pngImageClass::decodeFile(const string &filename)
	{
		holder<fileClass> f = newFile(filename, fileMode(true, false));
		memoryBuffer buffer(numeric_cast<uintPtr>(f->size()));
		f->read(buffer.data(), buffer.size());
		f->close();
		decodeBuffer(buffer);
	}

	void pngImageClass::decodeFile(const string &filename, uint32 channels, uint32 bpc)
	{
		holder<fileClass> f = newFile(filename, fileMode(true, false));
		memoryBuffer buffer(numeric_cast<uintPtr>(f->size()));
		f->read(buffer.data(), buffer.size());
		f->close();
		decodeBuffer(buffer, channels, bpc);
	}

	uint32 pngImageClass::width() const
	{
		const pngBufferImpl *impl = (const pngBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->mem.data(), "png image not initialized");
		return impl->width;
	}

	uint32 pngImageClass::height() const
	{
		const pngBufferImpl *impl = (const pngBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->mem.data(), "png image not initialized");
		return impl->height;
	}

	uint32 pngImageClass::channels() const
	{
		const pngBufferImpl *impl = (const pngBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->mem.data(), "png image not initialized");
		return impl->channels;
	}

	uint32 pngImageClass::bytesPerChannel() const
	{
		const pngBufferImpl *impl = (const pngBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->mem.data(), "png image not initialized");
		return impl->bytesPerChannel;
	}

	void *pngImageClass::bufferData()
	{
		pngBufferImpl *impl = (pngBufferImpl*)this;
		return impl->mem.data();
	}

	uintPtr pngImageClass::bufferSize() const
	{
		const pngBufferImpl *impl = (const pngBufferImpl*)this;
		return impl->mem.size();
	}

	float pngImageClass::value(uint32 x, uint32 y, uint32 c) const
	{
		const pngBufferImpl *impl = (const pngBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->mem.data(), "png image not initialized");
		CAGE_ASSERT_RUNTIME(x < impl->width && y < impl->height && c < impl->channels, x, impl->width, y, impl->height, c, impl->channels);
		switch (impl->bytesPerChannel)
		{
		case 1:
		{
			uint8 *d = (uint8*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			return *d / (float)detail::numeric_limits<uint8>::max();
		} break;
		case 2:
		{
			uint16 *d = (uint16*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			return *d / (float)detail::numeric_limits<uint16>::max();
		} break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid BPC in png image");
		}
	}

	void pngImageClass::value(uint32 x, uint32 y, uint32 c, float v)
	{
		pngBufferImpl *impl = (pngBufferImpl*)this;
		CAGE_ASSERT_RUNTIME(impl->mem.data(), "png image not initialized");
		CAGE_ASSERT_RUNTIME(x < impl->width && y < impl->height && c < impl->channels, x, impl->width, y, impl->height, c, impl->channels);
		v = clamp(v, 0.f, 1.f);
		switch (impl->bytesPerChannel)
		{
		case 1:
		{
			uint8 *d = (uint8*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			*d = numeric_cast<uint8>(v * detail::numeric_limits<uint8>::max());
		} break;
		case 2:
		{
			uint16 *d = (uint16*)impl->mem.data() + ((y * impl->width) + x) * impl->channels + c;
			*d = numeric_cast<uint16>(v * detail::numeric_limits<uint16>::max());
		} break;
		default:
			CAGE_THROW_CRITICAL(exception, "invalid BPC in png image");
		}
	}

	void pngImageClass::verticalFlip()
	{
		pngBufferImpl *impl = (pngBufferImpl*)this;
		uint32 lineSize = impl->bytesPerChannel * impl->channels * impl->width;
		uint32 swapsCount = impl->height / 2;
		memoryBuffer tmp;
		tmp.reallocate(lineSize);
		for (uint32 i = 0; i < swapsCount; i++)
		{
			detail::memcpy(tmp.data(), (char*)impl->mem.data() + i * lineSize, lineSize);
			detail::memcpy((char*)impl->mem.data() + i * lineSize, (char*)impl->mem.data() + (impl->height - i - 1) * lineSize, lineSize);
			detail::memcpy((char*)impl->mem.data() + (impl->height - i - 1) * lineSize, tmp.data(), lineSize);
		}
	}

	holder<pngImageClass> newPngImage()
	{
		return detail::systemArena().createImpl<pngImageClass, pngBufferImpl>();
	}
}