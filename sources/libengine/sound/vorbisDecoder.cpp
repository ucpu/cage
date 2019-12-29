#include <cage-core/core.h>
#include <cage-core/math.h>
#include "vorbisDecoder.h"

namespace cage
{
	namespace soundPrivat
	{
		vorbisDataStruct::vorbisDataStruct() : currentPosition(m)
		{
			callbacks.read_func = &read_func;
			callbacks.seek_func = &seek_func;
			callbacks.tell_func = &tell_func;
			callbacks.close_func = nullptr;
			ovf.datasource = nullptr;
		}

		vorbisDataStruct::~vorbisDataStruct()
		{
			clear();
		}

		void vorbisDataStruct::init(const void *buffer, uintPtr size)
		{
			this->buffer.resize(size);
			detail::memcpy(&this->buffer[0], buffer, size);
			currentPosition = 0;
			int res = ov_open_callbacks(this, &ovf, nullptr, 0, callbacks);
			if (res < 0)
				CAGE_THROW_ERROR(SystemError, "failed to open vorbis stream", res);
			CAGE_ASSERT(ovf.links == 1, ovf.links);
		}

		void vorbisDataStruct::clear()
		{
			if (!ovf.datasource)
				return;
			ov_clear(&ovf);
			ovf.datasource = nullptr;
			std::vector<char>().swap(this->buffer);
		}

		void vorbisDataStruct::read(float *output, uint32 index, uint32 frames)
		{
			CAGE_ASSERT(index + frames <= ov_pcm_total(&ovf, 0), index, frames, ov_pcm_total(&ovf, 0));

			int res = ov_pcm_seek(&ovf, index);
			if (res != 0)
				CAGE_THROW_ERROR(SystemError, "failed to seek in vorbis data", res);

			uint32 indexTarget = 0;
			while (indexTarget < frames)
			{
				float **pcm = nullptr;
				long res = ov_read_float(&ovf, &pcm, frames - indexTarget, nullptr);
				if (res < 0)
					CAGE_THROW_ERROR(SystemError, "failed to read vorbis data", res);
				if (res == 0)
					CAGE_THROW_ERROR(SystemError, "vorbis data truncated", res);
				for (uint32 f = 0; f < numeric_cast<uint32>(res); f++)
					for (uint32 ch = 0; ch < numeric_cast<uint32>(ovf.vi->channels); ch++)
						output[(indexTarget + f) * ovf.vi->channels + ch] = pcm[ch][f];
				indexTarget += res;
			}
		}

		void vorbisDataStruct::decode(uint32 &channels, uint32 &frames, uint32 &sampleRate, float *output)
		{
			channels = numeric_cast<uint32>(ovf.vi->channels);
			frames = numeric_cast<uint32>(ov_pcm_total(&ovf, 0));
			sampleRate = numeric_cast<uint32>(ovf.vi->rate);
			if (!output)
				return;
			uint32 offset = 0;
			while (true)
			{
				float **pcm = nullptr;
				long res = ov_read_float(&ovf, &pcm, 1024, nullptr);
				if (res < 0)
					CAGE_THROW_ERROR(SystemError, "failed to read vorbis data", res);
				if (res == 0)
					break;
				for (uint32 f = 0; f < numeric_cast<uint32>(res); f++)
					for (uint32 ch = 0; ch < channels; ch++)
						output[(offset + f) * channels + ch] = pcm[ch][f];
				offset += res;
			}
		}

		size_t vorbisDataStruct::read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
		{
			CAGE_ASSERT(size == 1 || nmemb == 1, size, nmemb);
			vorbisDataStruct *impl = (vorbisDataStruct*)datasource;
			size_t r = size * nmemb;
			r = min(r, impl->buffer.size() - impl->currentPosition);
			CAGE_ASSERT(r + impl->currentPosition <= impl->buffer.size(), r, impl->currentPosition, impl->buffer.size());
			if (r > 0)
				detail::memcpy(ptr, &impl->buffer[impl->currentPosition], r);
			impl->currentPosition += r;
			return r;
		}

		int vorbisDataStruct::seek_func(void *datasource, ogg_int64_t offset, int whence)
		{
			vorbisDataStruct *impl = (vorbisDataStruct*)datasource;
			switch (whence)
			{
			case SEEK_SET:
				impl->currentPosition = numeric_cast<size_t>(offset);
				break;
			case SEEK_CUR:
				impl->currentPosition += numeric_cast<size_t>(offset);
				break;
			case SEEK_END:
				impl->currentPosition = impl->buffer.size() + numeric_cast<size_t>(offset);
				break;
			}
			return 0;
		}

		long vorbisDataStruct::tell_func(void *datasource)
		{
			vorbisDataStruct *impl = (vorbisDataStruct*)datasource;
			return numeric_cast<long>(impl->currentPosition);
		}
	}
}
