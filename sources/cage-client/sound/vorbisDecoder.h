#include <vector>

#define NULL 0
#include <vorbis/vorbisfile.h>
#undef NULL

namespace cage
{
	namespace soundPrivat
	{
		struct vorbisDataStruct
		{
			vorbisDataStruct();
			~vorbisDataStruct();
			void init(void *buffer, uintPtr size);
			void clear();
			void read(float *output, uint32 index, uint32 frames);
			void decode(uint32 &channels, uint32 &frames, uint32 &sampleRate, float *output);

		private:
			std::vector<char> buffer;
			ov_callbacks callbacks;
			size_t currentPosition;
			OggVorbis_File ovf;

			static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource);
			static int seek_func(void *datasource, ogg_int64_t offset, int whence);
			static long tell_func(void *datasource);
		};
	}
}