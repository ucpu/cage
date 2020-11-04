#include "processor.h"

#include <dr_libs/dr_flac.h>
#include <dr_libs/dr_wav.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>

namespace
{
	SoundSourceHeader sds;
	MemoryBuffer buf1;

	struct VorbisEncoder
	{
		File *f;

		ogg_stream_state os;
		vorbis_info vi;
		vorbis_dsp_state v;
		vorbis_comment vc;
		vorbis_block vb;
		ogg_packet p;
		ogg_page og;
		int ret;

		explicit VorbisEncoder(File *f) : f(f) {}

		void processBlock()
		{
			while (true)
			{
				int moreBlocks = vorbis_analysis_blockout(&v, &vb);
				if (moreBlocks < 0)
					CAGE_THROW_ERROR(SystemError, "vorbis_analysis_blockout", moreBlocks);
				if (moreBlocks == 0)
					break;
				ret = vorbis_analysis(&vb, nullptr);
				if (ret != 0)
					CAGE_THROW_ERROR(SystemError, "vorbis_analysis", ret);
				ret = vorbis_bitrate_addblock(&vb);
				if (ret != 0)
					CAGE_THROW_ERROR(SystemError, "vorbis_bitrate_addblock", ret);
				while (true)
				{
					int morePackets = vorbis_bitrate_flushpacket(&v, &p);
					if (morePackets < 0)
						CAGE_THROW_ERROR(SystemError, "vorbis_bitrate_flushpacket", morePackets);
					if (morePackets == 0)
						break;
					ret = ogg_stream_packetin(&os, &p);
					if (ret != 0)
						CAGE_THROW_ERROR(SystemError, "ogg_stream_packetin p", ret);
					while (true)
					{
						ret = ogg_stream_pageout(&os, &og);
						if (ret == 0)
							break;
						f->write({ (char*)og.header, (char*)og.header + og.header_len });
						f->write({ (char*)og.body, (char*)og.body + og.body_len });
					}
				}
			}
		}

		void encode()
		{
			ret = ogg_stream_init(&os, 1);
			if (ret != 0)
				CAGE_THROW_ERROR(SystemError, "ogg_stream_init", ret);
			vorbis_info_init(&vi);
			ret = vorbis_encode_init_vbr(&vi, sds.channels, sds.sampleRate, toFloat(properties("compressQuality")));
			if (ret != 0)
				CAGE_THROW_ERROR(SystemError, "vorbis_encode_init_vbr", ret);
			vorbis_comment_init(&vc);
			vorbis_comment_add_tag(&vc, "ENCODER", "cage-asset-processor");
			ret = vorbis_analysis_init(&v, &vi);
			if (ret != 0)
				CAGE_THROW_ERROR(SystemError, "vorbis_analysis_init", ret);
			ret = vorbis_block_init(&v, &vb);
			if (ret != 0)
				CAGE_THROW_ERROR(SystemError, "vorbis_block_init", ret);
			{
				ogg_packet a, b, c;
				ret = vorbis_analysis_headerout(&v, &vc, &a, &b, &c);
				if (ret != 0)
					CAGE_THROW_ERROR(SystemError, "vorbis_analysis_headerout", ret);
				ret = ogg_stream_packetin(&os, &a);
				if (ret != 0)
					CAGE_THROW_ERROR(SystemError, "ogg_stream_packetin a", ret);
				ret = ogg_stream_packetin(&os, &b);
				if (ret != 0)
					CAGE_THROW_ERROR(SystemError, "ogg_stream_packetin b", ret);
				ret = ogg_stream_packetin(&os, &c);
				if (ret != 0)
					CAGE_THROW_ERROR(SystemError, "ogg_stream_packetin c", ret);
				while (true)
				{
					ret = ogg_stream_flush(&os, &og);
					if (ret == 0)
						break;
					f->write({ (char*)og.header, (char*)og.header + og.header_len });
					f->write({ (char*)og.body, (char*)og.body + og.body_len });
				}
			}
			uint32 offset = 0;
			uint32 frames = sds.frames;
			while (frames > 0)
			{
				uint32 wrt = min(frames, 1024u);
				float **buf2 = vorbis_analysis_buffer(&v, wrt);
				if (!buf2)
					CAGE_THROW_ERROR(Exception, "vorbis_analysis_buffer");
				for (uint32 f = 0; f < wrt; f++)
				{
					for (uint32 c = 0; c < sds.channels; c++)
						buf2[c][f] = ((float*)buf1.data())[(offset + f) * sds.channels + c];
				}
				ret = vorbis_analysis_wrote(&v, wrt);
				if (ret != 0)
					CAGE_THROW_ERROR(SystemError, "vorbis_analysis_wrote 1", ret);
				processBlock();
				offset += wrt;
				frames -= wrt;
			}
			ret = vorbis_analysis_wrote(&v, 0);
			if (ret != 0)
				CAGE_THROW_ERROR(SystemError, "vorbis_analysis_wrote 2", ret);
			processBlock();
			ogg_stream_clear(&os);
			vorbis_block_clear(&vb);
			vorbis_dsp_clear(&v);
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
		}
	};

	void decodeFlac()
	{
		unsigned int channels;
		unsigned int sampleRate;
		drflac_uint64 totalSampleCount;
		float* pSampleData = drflac_open_file_and_read_pcm_frames_f32(pathJoin(inputDirectory, inputFile).c_str(), &channels, &sampleRate, &totalSampleCount, nullptr);
		if (!pSampleData)
			CAGE_THROW_ERROR(Exception, "failed to read flac file");
		sds.channels = channels;
		sds.frames = numeric_cast<uint32>(totalSampleCount / channels);
		sds.sampleRate = sampleRate;
		buf1.allocate(sds.channels * sds.frames * sizeof(float));
		detail::memcpy(buf1.data(), pSampleData, buf1.size());
		drflac_free(pSampleData, nullptr);
	}

	void decodeWav()
	{
		unsigned int channels;
		unsigned int sampleRate;
		drwav_uint64 totalSampleCount;
		float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(pathJoin(inputDirectory, inputFile).c_str(), &channels, &sampleRate, &totalSampleCount, nullptr);
		if (!pSampleData)
			CAGE_THROW_ERROR(Exception, "failed to read wav file");
		sds.channels = channels;
		sds.frames = numeric_cast<uint32>(totalSampleCount / channels);
		sds.sampleRate = sampleRate;
		buf1.allocate(sds.channels * sds.frames * sizeof(float));
		detail::memcpy(buf1.data(), pSampleData, buf1.size());
		drwav_free(pSampleData, nullptr);
	}

	void decodeVorbis()
	{
		OggVorbis_File vf;
		if (ov_open_callbacks(fopen(pathJoin(inputDirectory, inputFile).c_str(), "rb"), &vf, NULL, 0, OV_CALLBACKS_DEFAULT) < 0)
			CAGE_THROW_ERROR(Exception, "failed to open vorbis stream");
		try
		{
			vorbis_info *vi = ov_info(&vf, -1);
			sds.channels = vi->channels;
			sds.sampleRate = vi->rate;
			sds.frames = numeric_cast<uint32>(ov_pcm_total(&vf, -1));
			buf1.allocate(sds.channels * sds.frames * sizeof(float));
			uint32 offset = 0;
			while (true)
			{
				float **pcm = nullptr;
				int bitstream = 0;
				long ret = ov_read_float(&vf, &pcm, sds.frames - offset, &bitstream);
				if (ret == 0)
					break;
				if (ret < 0)
					CAGE_THROW_ERROR(Exception, "corrupted vorbis stream");
				for (uint32 f = 0; f < numeric_cast<unsigned long>(ret); f++)
				{
					for (uint32 c = 0; c < sds.channels; c++)
						((float*)buf1.data())[(offset + f) * sds.channels + c] = pcm[c][f];
				}
				offset += ret;
			}
		}
		catch (...)
		{
			ov_clear(&vf);
			throw;
		}
		ov_clear(&vf);
	}

	void decodeAll()
	{
		if (isPattern(inputFile, "", "", ".flac"))
			decodeFlac();
		else if (isPattern(inputFile, "", "", ".wav"))
			decodeWav();
		else
			decodeVorbis();
	}
}

void processSound()
{
	writeLine(string("use=") + inputFile);

	decodeAll();

	if (toBool(properties("loopBefore")))
		sds.flags = sds.flags | SoundFlags::LoopBeforeStart;
	if (toBool(properties("loopAfter")))
		sds.flags = sds.flags | SoundFlags::LoopAfterEnd;
	if (sds.frames * sds.channels * sizeof(float) < toUint32(properties("compressThreshold")))
		sds.soundType = SoundTypeEnum::RawRaw;
	else if (toBool(properties("stream")))
		sds.soundType = SoundTypeEnum::CompressedCompressed;
	else
		sds.soundType = SoundTypeEnum::CompressedRaw;

	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "flags: " + (uint32)sds.flags);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "frames: " + sds.frames);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "channels: " + sds.channels);
	CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "samplerate: " + sds.sampleRate);
	switch (sds.soundType)
	{
	case SoundTypeEnum::RawRaw:
		CAGE_LOG(SeverityEnum::Info, logComponentName, "sound type: raw file, raw play");
		break;
	case SoundTypeEnum::CompressedRaw:
		CAGE_LOG(SeverityEnum::Info, logComponentName, "sound type: compressed file, raw play");
		break;
	case SoundTypeEnum::CompressedCompressed:
		CAGE_LOG(SeverityEnum::Info, logComponentName, "sound type: compressed file, compressed play");
		break;
	default:
		CAGE_THROW_CRITICAL(Exception, "invalid sound type");
	}

	AssetHeader h = initializeAssetHeader();
	h.originalSize = sizeof(SoundSourceHeader) + sds.frames * sds.channels * sizeof(float);

	Holder<File> f = writeFile(outputFileName);
	f->write(bufferView(h));
	f->write(bufferView(sds));
	switch (sds.soundType)
	{
	case SoundTypeEnum::RawRaw:
	{
		f->write(buf1);
	} break;
	case SoundTypeEnum::CompressedRaw:
	case SoundTypeEnum::CompressedCompressed:
	{
		VorbisEncoder ves(f.get());
		ves.encode();
		uint32 oggSize = numeric_cast<uint32>(f->size() - sizeof(SoundSourceHeader) - sizeof(AssetHeader));
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "original size: " + h.originalSize + " bytes");
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "compressed size: " + oggSize + " bytes");
		CAGE_LOG(SeverityEnum::Info, logComponentName, stringizer() + "compression ratio: " + (oggSize / (float)h.originalSize));
		switch (sds.soundType)
		{
		case SoundTypeEnum::CompressedRaw:
			h.compressedSize = oggSize + sizeof(SoundSourceHeader);
			break;
		case SoundTypeEnum::CompressedCompressed:
			h.compressedSize = oggSize + sizeof(SoundSourceHeader);
			h.originalSize = 0; // the sound will not be decoded on asset load, so do not allocate space for it
			break;
		case SoundTypeEnum::RawRaw:
			break; // do nothing here
		}
		f->seek(0);
		f->write(bufferView(h));
	} break;
	default:
		CAGE_THROW_CRITICAL(Exception, "invalid sound type");
	}
	f->close();

	// preview sound
	if (configGetBool("cage-asset-processor/sound/preview"))
	{
		CAGE_THROW_ERROR(NotImplemented, "sound preview is not currently implemented");
	}
}

void analyzeSound()
{
	try
	{
		decodeAll();
		writeLine("cage-begin");
		writeLine("scheme=sound");
		writeLine(string() + "asset=" + inputFile);
		writeLine("cage-end");
	}
	catch (...)
	{
		// do nothing
	}
}
