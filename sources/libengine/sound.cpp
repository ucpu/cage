#include <cage-core/audio.h>

#include <cage-engine/sound.h>

namespace cage
{
	namespace
	{
		class SoundImpl : public Sound
		{
		public:
			Holder<Audio> stream;
			sintPtr length = 0;
			sint32 channels = 0;
			sint32 sampleRate = 0;

			void initialize(Holder<Audio> &&audio)
			{
				stream = std::move(audio);
				length = stream->frames();
				channels = stream->channels();
				sampleRate = stream->sampleRate();
			}

			void decodeOne(PointerRange<float> buffer, sintPtr bufferOffset, sintPtr streamOffset, sintPtr frames) const
			{
				CAGE_ASSERT(bufferOffset >= 0 && streamOffset >= 0 && frames >= 0);
				CAGE_ASSERT(streamOffset + frames <= length);
				CAGE_ASSERT((bufferOffset + frames) * channels <= numeric_cast<sintPtr>(buffer.size()));
				stream->decode(streamOffset, { buffer.data() + channels * bufferOffset, buffer.data() + channels * (bufferOffset + frames) });
			}

			void decodeLoop(PointerRange<float> buffer, sintPtr bufferOffset, sintPtr streamOffset, sintPtr frames) const
			{
				CAGE_ASSERT(bufferOffset >= 0 && frames >= 0);
				CAGE_ASSERT((bufferOffset + frames) * channels <= numeric_cast<sintPtr>(buffer.size()));
				if (streamOffset < 0)
					streamOffset += (-streamOffset / length + 1) * length;
				while (frames)
				{
					CAGE_ASSERT(streamOffset >= 0);
					streamOffset %= length;
					const sintPtr f = min(streamOffset + frames, length) - streamOffset;
					CAGE_ASSERT(f > 0 && f <= frames && streamOffset + f <= length);
					decodeOne(buffer, bufferOffset, streamOffset, f);
					bufferOffset += f;
					streamOffset += f;
					frames -= f;
				}
			}

			void zeroFill(PointerRange<float> buffer, sintPtr bufferOffset, sintPtr frames) const
			{
				CAGE_ASSERT(bufferOffset >= 0 && frames >= 0);
				CAGE_ASSERT((bufferOffset + frames) * channels <= numeric_cast<sintPtr>(buffer.size()));
				detail::memset(buffer.data() + channels * bufferOffset, 0, channels * frames * sizeof(float));
			}

			void resolveLooping(PointerRange<float> buffer, sintPtr startFrame, sintPtr frames) const
			{
				CAGE_ASSERT(frames >= 0);
				CAGE_ASSERT(frames * channels == numeric_cast<sintPtr>(buffer.size()));

				sintPtr bufferOffset = 0;

				if (startFrame < 0)
				{ // before start
					const sintPtr r = min(-startFrame, frames);
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
					const sintPtr r = min(length - startFrame, frames);
					decodeOne(buffer, bufferOffset, startFrame, r);
					bufferOffset += r;
					frames -= r;
					startFrame += r;
				}

				if (startFrame >= length && frames)
				{ // after end
					const sintPtr r = frames;
					if (loopAfterEnd)
						decodeLoop(buffer, bufferOffset, startFrame, r);
					else
						zeroFill(buffer, bufferOffset, r);
					bufferOffset += r;
					frames -= r;
					startFrame += r;
				}

				CAGE_ASSERT(bufferOffset * channels == numeric_cast<sintPtr>(buffer.size()));
				CAGE_ASSERT(frames == 0);
			}

			void decode(sintPtr startFrame, PointerRange<float> buffer)
			{
				CAGE_ASSERT(buffer.size() % channels == 0);
				resolveLooping(buffer, startFrame, buffer.size() / channels);
			}

			void process(const SoundCallbackData &data)
			{
				if (data.channels != channels || data.sampleRate != sampleRate)
					CAGE_THROW_ERROR(Exception, "unmatched channels or sample rate");
				resolveLooping(data.buffer, numeric_cast<sintPtr>(data.time * sampleRate / 1000000), data.frames);
			}
		};
	}

	void Sound::initialize(Holder<Audio> &&audio)
	{
		SoundImpl *impl = (SoundImpl *)this;
		impl->initialize(std::move(audio));
	}

	uintPtr Sound::frames() const
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

	void Sound::decode(sintPtr startFrame, PointerRange<float> buffer)
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
		return systemArena().createImpl<Sound, SoundImpl>();
	}
}
