#include <cage-core/audio.h>
#include <cage-core/sampleRateConverter.h>

#include <cage-engine/sound.h>

#include <vector>

namespace cage
{
	namespace
	{
		class SoundImpl : public Sound
		{
		public:
			Holder<AudioStream> stream;
			Holder<SampleRateConverter> conv;
			std::vector<float> buffer;
			uint64 length = 0;
			uint32 channels = 0;
			uint32 sampleRate = 0;

			void initialize(Holder<Audio> &&audio)
			{
				stream = newAudioStream(templates::move(audio));
				length = stream->source()->frames();
				channels = stream->source()->channels();
				sampleRate = stream->source()->sampleRate();
				conv = newSampleRateConverter(channels);
			}

			void decodeOne(uint64 bufferOffset, sint64 streamOffset, uint64 frames)
			{
				CAGE_ASSERT(streamOffset >= 0);
				CAGE_ASSERT(streamOffset + frames <= stream->source()->frames());
				CAGE_ASSERT((bufferOffset + frames) * channels <= buffer.size());
				stream->decode(streamOffset, { buffer.data() + channels * bufferOffset, buffer.data() + channels * (bufferOffset + frames) });
			}

			void decodeLoop(uint64 bufferOffset, sint64 streamOffset, uint64 frames)
			{
				if (streamOffset < 0)
					streamOffset += ((length - streamOffset) / length) * length;
				while (frames)
				{
					CAGE_ASSERT(streamOffset >= 0);
					streamOffset %= length;
					const uint64 f = min(streamOffset + frames, length) - streamOffset; // streamOffset + frames > length ? length - frames - streamOffset : frames;
					CAGE_ASSERT(f > 0 && f <= frames && streamOffset + f <= length);
					decodeOne(bufferOffset, streamOffset, f);
					bufferOffset += f;
					streamOffset += f;
					frames -= f;
				}
			}

			void zeroFill(uint64 bufferOffset, uint64 frames)
			{
				CAGE_ASSERT((bufferOffset + frames) * channels <= buffer.size());
				for (uint64 i = 0; i < frames; i++)
					for (uint32 ch = 0; ch < channels; ch++)
						buffer[(bufferOffset + i) * channels + ch] = 0;
			}

			void resolveLooping(sint64 startFrame, uint64 frames)
			{
				buffer.resize(channels * frames);
				uint64 bufferOffset = 0;
				if (!loopBeforeStart && startFrame < 0)
				{
					zeroFill(0, -startFrame);
					bufferOffset += -startFrame;
					frames -= -startFrame;
					startFrame = 0;
				}
				CAGE_ASSERT((bufferOffset + frames) * channels == buffer.size());
				if (!loopAfterEnd && startFrame + frames > length)
				{
					const uint64 cnt = startFrame + frames - length;
					zeroFill(bufferOffset + frames - cnt, cnt);
					frames -= cnt;
				}
				decodeLoop(bufferOffset, startFrame, frames);
			}

			void process(const SoundCallbackData &data)
			{
				if (data.channels != channels)
					CAGE_THROW_ERROR(NotImplemented, "unmatched number of channels");
				resolveLooping(data.time * sampleRate / 1000000, data.frames);
				conv->convert(buffer, data.buffer, data.sampleRate / (double)sampleRate);
			}
		};
	}

	void Sound::initialize(Holder<Audio> &&audio)
	{
		SoundImpl *impl = (SoundImpl *)this;
		impl->initialize(templates::move(audio));
	}

	void Sound::process(const SoundCallbackData &data)
	{
		SoundImpl *impl = (SoundImpl *)this;
		impl->process(data);
	}

	Holder<Sound> newSound()
	{
		return detail::systemArena().createImpl<Sound, SoundImpl>();
	}
}
