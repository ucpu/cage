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
			sint64 length = 0;
			sint32 channels = 0;
			sint32 sampleRate = 0;

			void initialize(Holder<Audio> &&audio)
			{
				stream = newAudioStream(templates::move(audio));
				length = stream->source()->frames();
				channels = stream->source()->channels();
				sampleRate = stream->source()->sampleRate();
			}

			void decodeOne(PointerRange<float> buffer, sint64 bufferOffset, sint64 streamOffset, sint64 frames) const
			{
				CAGE_ASSERT(bufferOffset >= 0 && streamOffset >= 0 && frames >= 0);
				CAGE_ASSERT(streamOffset + frames <= length);
				CAGE_ASSERT((bufferOffset + frames) * channels <= numeric_cast<sint64>(buffer.size()));
				stream->decode(streamOffset, { buffer.data() + channels * bufferOffset, buffer.data() + channels * (bufferOffset + frames) });
			}

			void decodeLoop(PointerRange<float> buffer, sint64 bufferOffset, sint64 streamOffset, sint64 frames) const
			{
				CAGE_ASSERT(bufferOffset >= 0 && frames >= 0);
				CAGE_ASSERT((bufferOffset + frames) * channels <= numeric_cast<sint64>(buffer.size()));
				if (streamOffset < 0)
					streamOffset += (-streamOffset / length + 1) * length;
				while (frames)
				{
					CAGE_ASSERT(streamOffset >= 0);
					streamOffset %= length;
					const sint64 f = min(streamOffset + frames, length) - streamOffset;
					CAGE_ASSERT(f > 0 && f <= frames && streamOffset + f <= length);
					decodeOne(buffer, bufferOffset, streamOffset, f);
					bufferOffset += f;
					streamOffset += f;
					frames -= f;
				}
			}

			void zeroFill(PointerRange<float> buffer, sint64 bufferOffset, sint64 frames) const
			{
				CAGE_ASSERT(bufferOffset >= 0 && frames >= 0);
				CAGE_ASSERT((bufferOffset + frames) * channels <= numeric_cast<sint64>(buffer.size()));
				detail::memset(buffer.data() + channels * bufferOffset, 0, channels * frames * sizeof(float));
			}

			void resolveLooping(PointerRange<float> buffer, sint64 startFrame, sint64 frames) const
			{
				CAGE_ASSERT(frames >= 0);
				CAGE_ASSERT(frames * channels == numeric_cast<sint64>(buffer.size()));

				sint64 bufferOffset = 0;

				if (startFrame < 0)
				{ // before start
					const sint64 r = min(-startFrame, frames);
					if (loopBeforeStart)
						decodeLoop(buffer, bufferOffset, startFrame, r);
					else
						zeroFill(buffer, bufferOffset, r);
					bufferOffset += r;
					frames -= r;
					startFrame += r;
				}

				if (startFrame < length && frames)
				{ // inside
					const sint64 r = min(length - startFrame, frames);
					decodeOne(buffer, bufferOffset, startFrame, r);
					bufferOffset += r;
					frames -= r;
					startFrame += r;
				}

				if (startFrame >= length && frames)
				{ // after end
					const sint64 r = frames;
					if (loopAfterEnd)
						decodeLoop(buffer, bufferOffset, startFrame, r);
					else
						zeroFill(buffer, bufferOffset, r);
					bufferOffset += r;
					frames -= r;
					startFrame += r;
				}

				CAGE_ASSERT(bufferOffset * channels == numeric_cast<sint64>(buffer.size()));
				CAGE_ASSERT(frames == 0);
			}

			void decode(sint64 startFrame, PointerRange<float> buffer)
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

	void Sound::decode(sint64 startFrame, PointerRange<float> buffer)
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
