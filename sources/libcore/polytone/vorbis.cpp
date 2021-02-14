#include "polytone.h"

#include <cage-core/math.h>
#include <cage-core/serialization.h>
#include <cage-core/files.h>

#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

#include <utility> // std::swap

namespace cage
{
	namespace
	{
		struct VorbisDecoder : Immovable
		{
			uint64 frames = 0;
			uint32 channels = 0;
			uint32 sampleRate = 0;

			VorbisDecoder(Holder<File> &&file) : file(templates::move(file))
			{
				callbacks.read_func = &read_func;
				callbacks.seek_func = &seek_func;
				callbacks.tell_func = &tell_func;
				if (ov_open_callbacks(this, &ovf, nullptr, 0, callbacks) < 0)
					CAGE_THROW_ERROR(Exception, "failed initialization in decoding vorbis sound stream");
				CAGE_ASSERT(ovf.links == 1);
				frames = numeric_cast<uint32>(ov_pcm_total(&ovf, 0));
				channels = numeric_cast<uint32>(ovf.vi->channels);
				sampleRate = numeric_cast<uint32>(ovf.vi->rate);
			}

			~VorbisDecoder()
			{
				ov_clear(&ovf);
				ovf = {};
			}

			void seekFrame(uint64 frame)
			{
				if (ov_pcm_seek(&ovf, frame) != 0)
					CAGE_THROW_ERROR(Exception, "failed seek in decoding vorbis sound stream");
			}

			void seekTime(uint64 time)
			{
				if (ov_time_seek(&ovf, time * 1e-6) != 0)
					CAGE_THROW_ERROR(Exception, "failed seek in decoding vorbis sound stream");
			}

			void decode(PointerRange<float> buffer)
			{
				CAGE_ASSERT(buffer.size() % ovf.vi->channels == 0);
				const uint64 frames = buffer.size() / ovf.vi->channels;
				uint64 indexTarget = 0;
				while (indexTarget < frames)
				{
					float **pcm = nullptr;
					long res = ov_read_float(&ovf, &pcm, numeric_cast<int>(min(frames - indexTarget, uint64(1073741824))), nullptr);
					if (res <= 0)
						CAGE_THROW_ERROR(Exception, "failed read in decoding vorbis sound stream");
					for (long f = 0; f < res; f++)
						for (uint32 ch = 0; ch < numeric_cast<uint32>(ovf.vi->channels); ch++)
							buffer[(indexTarget + f) * ovf.vi->channels + ch] = pcm[ch][f];
					indexTarget += res;
				}
			}

		private:
			Holder<File> file;
			ov_callbacks callbacks = {};
			OggVorbis_File ovf = {};

			static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
			{
				CAGE_ASSERT(size == 1 || nmemb == 1);
				VorbisDecoder *impl = (VorbisDecoder *)datasource;
				size_t r = size * nmemb;
				r = min(r, impl->file->size() - impl->file->tell());
				if (r > 0)
					impl->file->read({ (char *)ptr, (char *)ptr + r });
				return r;
			}

			static int seek_func(void *datasource, ogg_int64_t offset, int whence)
			{
				VorbisDecoder *impl = (VorbisDecoder *)datasource;
				switch (whence)
				{
				case SEEK_SET:
					impl->file->seek(offset);
					break;
				case SEEK_CUR:
					impl->file->seek(impl->file->tell() + offset);
					break;
				case SEEK_END:
					impl->file->seek(impl->file->size() + offset);
					break;
				}
				return 0;
			}

			static long tell_func(void *datasource)
			{
				VorbisDecoder *impl = (VorbisDecoder *)datasource;
				return numeric_cast<long>(impl->file->tell());
			}
		};

		struct VorbisEncoder : Immovable
		{
			const MemoryBuffer &inputBuffer;
			MemoryBuffer outputBuffer;

			VorbisEncoder(const PolytoneImpl *src, float compressQuality = 1) : inputBuffer(src->mem), compressQuality(compressQuality)
			{
				if (src->format != PolytoneFormatEnum::Float)
					CAGE_THROW_ERROR(Exception, "invalid format for encoding vorbis sound stream");
				frames = src->frames;
				channels = src->channels;
				sampleRate = src->sampleRate;
			}

			void encode()
			{
				if (ogg_stream_init(&os, 1) != 0)
					CAGE_THROW_ERROR(Exception, "ogg_stream_init");
				vorbis_info_init(&vi);
				if (vorbis_encode_init_vbr(&vi, channels, sampleRate, compressQuality) != 0)
					CAGE_THROW_ERROR(Exception, "vorbis_encode_init_vbr");
				vorbis_comment_init(&vc);
				vorbis_comment_add_tag(&vc, "ENCODER", "cage-asset-processor");
				if (vorbis_analysis_init(&v, &vi) != 0)
					CAGE_THROW_ERROR(Exception, "vorbis_analysis_init");
				if (vorbis_block_init(&v, &vb) != 0)
					CAGE_THROW_ERROR(Exception, "vorbis_block_init");
				{
					ogg_packet a, b, c;
					if (vorbis_analysis_headerout(&v, &vc, &a, &b, &c) != 0)
						CAGE_THROW_ERROR(Exception, "vorbis_analysis_headerout");
					if (ogg_stream_packetin(&os, &a) != 0)
						CAGE_THROW_ERROR(Exception, "ogg_stream_packetin a");
					if (ogg_stream_packetin(&os, &b) != 0)
						CAGE_THROW_ERROR(Exception, "ogg_stream_packetin b");
					if (ogg_stream_packetin(&os, &c) != 0)
						CAGE_THROW_ERROR(Exception, "ogg_stream_packetin c");
					while (true)
					{
						if (ogg_stream_flush(&os, &og) == 0)
							break;
						ser.write({ (char *)og.header, (char *)og.header + og.header_len });
						ser.write({ (char *)og.body, (char *)og.body + og.body_len });
					}
				}
				uint64 offset = 0;
				while (frames > 0)
				{
					uint64 wrt = min(frames, uint64(1024));
					float **buf2 = vorbis_analysis_buffer(&v, numeric_cast<int>(wrt));
					if (!buf2)
						CAGE_THROW_ERROR(Exception, "vorbis_analysis_buffer");
					PointerRange<const float> src = bufferCast<const float, const char>(inputBuffer);
					for (uint32 f = 0; f < wrt; f++)
					{
						for (uint32 c = 0; c < channels; c++)
							buf2[c][f] = src[(offset + f) * channels + c];
					}
					if (vorbis_analysis_wrote(&v, numeric_cast<int>(wrt)) != 0)
						CAGE_THROW_ERROR(Exception, "vorbis_analysis_wrote 1");
					processBlock();
					offset += wrt;
					frames -= wrt;
				}
				if (vorbis_analysis_wrote(&v, 0) != 0)
					CAGE_THROW_ERROR(Exception, "vorbis_analysis_wrote 2");
				processBlock();
			}

			~VorbisEncoder()
			{
				ogg_stream_clear(&os);
				vorbis_block_clear(&vb);
				vorbis_dsp_clear(&v);
				vorbis_comment_clear(&vc);
				vorbis_info_clear(&vi);
			}

		private:
			Serializer ser = Serializer(outputBuffer);
			uint64 frames = 0;
			uint32 channels = 0;
			uint32 sampleRate = 0;
			float compressQuality = 1;

			ogg_stream_state os = {};
			vorbis_info vi = {};
			vorbis_dsp_state v = {};
			vorbis_comment vc = {};
			vorbis_block vb = {};
			ogg_packet p = {};
			ogg_page og = {};

			void processBlock()
			{
				while (true)
				{
					int moreBlocks = vorbis_analysis_blockout(&v, &vb);
					if (moreBlocks < 0)
						CAGE_THROW_ERROR(Exception, "vorbis_analysis_blockout");
					if (moreBlocks == 0)
						break;
					if (vorbis_analysis(&vb, nullptr) != 0)
						CAGE_THROW_ERROR(Exception, "vorbis_analysis");
					if (vorbis_bitrate_addblock(&vb) != 0)
						CAGE_THROW_ERROR(Exception, "vorbis_bitrate_addblock");
					while (true)
					{
						int morePackets = vorbis_bitrate_flushpacket(&v, &p);
						if (morePackets < 0)
							CAGE_THROW_ERROR(Exception, "vorbis_bitrate_flushpacket");
						if (morePackets == 0)
							break;
						if (ogg_stream_packetin(&os, &p) != 0)
							CAGE_THROW_ERROR(Exception, "ogg_stream_packetin p");
						while (true)
						{
							if (ogg_stream_pageout(&os, &og) == 0)
								break;
							ser.write({ (char *)og.header, (char *)og.header + og.header_len });
							ser.write({ (char *)og.body, (char *)og.body + og.body_len });
						}
					}
				}
			}
		};
	}

	void vorbisDecode(PointerRange<const char> inBuffer, PolytoneImpl *impl)
	{
		VorbisDecoder dec(newFileBuffer(inBuffer));
		impl->mem.resize(0); // avoid copying
		impl->mem.resize(inBuffer.size());
		detail::memcpy(impl->mem.data(), inBuffer.data(), inBuffer.size());
		impl->frames = dec.frames;
		impl->channels = dec.channels;
		impl->sampleRate = dec.sampleRate;
		impl->format = PolytoneFormatEnum::Vorbis;
	}

	MemoryBuffer vorbisEncode(const PolytoneImpl *impl)
	{
		if (impl->format == PolytoneFormatEnum::Vorbis)
			return impl->mem.copy();
		else
		{
			VorbisEncoder enc(impl);
			enc.encode();
			return templates::move(enc.outputBuffer);
		}
	}

	void vorbisConvertFormat(PolytoneImpl *snd, PolytoneFormatEnum format)
	{
		CAGE_ASSERT(snd->format != format);
		CAGE_ASSERT(snd->format == PolytoneFormatEnum::Vorbis || format == PolytoneFormatEnum::Vorbis);
		if (snd->format == PolytoneFormatEnum::Vorbis)
		{
			// vorbis -> float -> format
			MemoryBuffer tmp;
			std::swap(tmp, snd->mem);
			VorbisDecoder dec(newFileBuffer(tmp, FileMode(true, false)));
			CAGE_ASSERT(dec.frames == snd->frames);
			CAGE_ASSERT(dec.channels == snd->channels);
			snd->initialize(dec.frames, dec.channels, dec.sampleRate, PolytoneFormatEnum::Float);
			dec.decode(bufferCast<float, char>(snd->mem));
			polytoneConvertFormat(snd, format);
		}
		else
		{
			// snd->format -> float -> vorbis
			polytoneConvertFormat(snd, PolytoneFormatEnum::Float);
			VorbisEncoder enc(snd);
			enc.encode();
			std::swap(snd->mem, enc.outputBuffer);
			snd->format = PolytoneFormatEnum::Vorbis;
		}
	}

	Holder<Polytone> vorbisExtract(const PolytoneImpl *src, uint64 offset, uint64 frames)
	{
		CAGE_ASSERT(src->format == PolytoneFormatEnum::Vorbis);
		CAGE_ASSERT(offset + frames <= src->frames);
		Holder<Polytone> poly = newPolytone();
		PolytoneImpl *dst = (PolytoneImpl *)+poly;
		dst->initialize(frames, src->channels, src->sampleRate, PolytoneFormatEnum::Float);
		VorbisDecoder dec(newFileBuffer(src->mem));
		dec.seekFrame(offset);
		dec.decode(bufferCast<float, char>(dst->mem));
		return poly;
	}
}
