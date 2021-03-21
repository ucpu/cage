#include <cage-core/audio.h>

#include <cage-engine/sound.h>

namespace cage
{
	namespace
	{
		class SoundImpl : public Sound
		{
		public:
			Holder<AudioStream> stream;
			uint64 length = 0;
			uint32 channels = 0;
			uint32 sampleRate = 0;

			void initialize(Holder<Audio> &&audio)
			{
				stream = newAudioStream(templates::move(audio));
				length = stream->source()->frames();
				channels = stream->source()->channels();
				sampleRate = stream->source()->sampleRate();
			}

			void decodeOne(PointerRange<float> buffer, uint64 bufferOffset, sint64 streamOffset, uint64 frames)
			{
				CAGE_ASSERT(streamOffset >= 0);
				CAGE_ASSERT(streamOffset + frames <= stream->source()->frames());
				CAGE_ASSERT((bufferOffset + frames) * channels <= buffer.size());
				stream->decode(streamOffset, { buffer.data() + channels * bufferOffset, buffer.data() + channels * (bufferOffset + frames) });
			}

			void decodeLoop(PointerRange<float> buffer, uint64 bufferOffset, sint64 streamOffset, uint64 frames)
			{
				if (streamOffset < 0)
					streamOffset += ((length - streamOffset) / length) * length;
				while (frames)
				{
					CAGE_ASSERT(streamOffset >= 0);
					streamOffset %= length;
					const uint64 f = min(streamOffset + frames, length) - streamOffset; // streamOffset + frames > length ? length - frames - streamOffset : frames;
					CAGE_ASSERT(f > 0 && f <= frames && streamOffset + f <= length);
					decodeOne(buffer, bufferOffset, streamOffset, f);
					bufferOffset += f;
					streamOffset += f;
					frames -= f;
				}
			}

			void zeroFill(PointerRange<float> buffer, uint64 bufferOffset, uint64 frames)
			{
				CAGE_ASSERT((bufferOffset + frames) * channels <= buffer.size());
				for (uint64 i = 0; i < frames; i++)
					for (uint32 ch = 0; ch < channels; ch++)
						buffer[(bufferOffset + i) * channels + ch] = 0;
			}

			void resolveLooping(PointerRange<float> buffer, sint64 startFrame, uint64 frames)
			{
				CAGE_ASSERT(buffer.size() == frames * channels);
				uint64 bufferOffset = 0;
				if (!loopBeforeStart && startFrame < 0)
				{
					zeroFill(buffer, 0, -startFrame);
					bufferOffset += -startFrame;
					frames -= -startFrame;
					startFrame = 0;
				}
				CAGE_ASSERT((bufferOffset + frames) * channels == buffer.size());
				if (!loopAfterEnd && startFrame + frames > length)
				{
					const uint64 cnt = startFrame + frames - length;
					zeroFill(buffer, bufferOffset + frames - cnt, cnt);
					frames -= cnt;
				}
				decodeLoop(buffer, bufferOffset, startFrame, frames);
			}

			void decode(uint64 startFrame, PointerRange<float> buffer)
			{
				CAGE_ASSERT(buffer.size() % channels == 0);
				resolveLooping(buffer, startFrame, buffer.size() / channels);
			}

			void process(const SoundCallbackData &data)
			{
				if (data.channels != channels || data.sampleRate != sampleRate)
					CAGE_THROW_ERROR(Exception, "unmatched channels or sample rate");
				resolveLooping(data.buffer, data.time * sampleRate / 1000000, data.frames);
			}
		};
	}

	void Sound::initialize(Holder<Audio> &&audio)
	{
		SoundImpl *impl = (SoundImpl *)this;
		impl->initialize(templates::move(audio));
	}

	uint64 Sound::frames() const
	{
		const SoundImpl *impl = (const SoundImpl *)this;
		return impl->length;
	}

	uint32 Sound::channels() const
	{
		const SoundImpl *impl = (const SoundImpl *)this;
		return impl->channels;
	}

	uint32 Sound::sampleRate() const
	{
		const SoundImpl *impl = (const SoundImpl *)this;
		return impl->sampleRate;
	}

	uint64 Sound::duration() const
	{
		const SoundImpl *impl = (const SoundImpl *)this;
		return 1000000 * frames() / sampleRate();
	}

	void Sound::decode(uint64 startFrame, PointerRange<float> buffer)
	{
		SoundImpl *impl = (SoundImpl *)this;
		impl->decode(startFrame, buffer);
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
