#include "audio.h"

#include <cage-core/serialization.h>
#include <cage-core/files.h>

#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>

namespace cage
{
	struct VorbisDecoder : Immovable
	{
		uintPtr frames = 0;
		uint32 channels = 0;
		uint32 sampleRate = 0;

		VorbisDecoder(Holder<File> &&file);
		~VorbisDecoder();
		void seek(uintPtr frame);
		uintPtr tell() const;
		void decode(PointerRange<float> buffer);

	private:
		Holder<File> file;
		ov_callbacks callbacks = {};
		OggVorbis_File ovf = {};

		static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource);
		static int seek_func(void *datasource, ogg_int64_t offset, int whence);
		static long tell_func(void *datasource);
	};

	struct VorbisEncoder : Immovable
	{
		const MemoryBuffer &inputBuffer;
		MemoryBuffer outputBuffer;

		VorbisEncoder(const AudioImpl *src, float compressQuality = 1);
		void encode();
		~VorbisEncoder();

	private:
		Serializer ser = Serializer(outputBuffer);
		uintPtr frames = 0;
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

		void processBlock();
	};
}
