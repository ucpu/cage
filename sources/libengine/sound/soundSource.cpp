#include <cage-core/memoryAllocators.h>

#include "vorbisDecoder.h"
#include "private.h"
#include "utilities.h"

#include <set>

namespace cage
{
	namespace
	{
		enum class DataTypeEnum : uint32
		{
			None = 0,
			Raw,
			Vorbis,
			Tone,
			Noise,
		};

		class SoundSourceImpl : public SoundSource, public BusInterface
		{
		public:
			std::set<MixingBus *> outputs;
			std::vector<float> rawData;
			std::vector<float> temporaryData1;
			std::vector<float> temporaryData2;
			std::vector<float> temporaryData3;
			uint32 channels = 0;
			uint32 frames = 0;
			uint32 sampleRate = 0;
			uint32 pitch = 0;
			DataTypeEnum dataType = DataTypeEnum::None;
			bool repeatBeforeStart = false;
			bool repeatAfterEnd = false;

			soundPrivat::VorbisData vorbisData;

			SoundSourceImpl() : BusInterface(Delegate<void(MixingBus*)>().bind<SoundSourceImpl, &SoundSourceImpl::busDestroyed>(this), Delegate<void(const SoundDataBuffer&)>().bind<SoundSourceImpl, &SoundSourceImpl::execute>(this))
			{}

			~SoundSourceImpl()
			{
				for (auto it = outputs.begin(), et = outputs.end(); it != et; it++)
					busRemoveInput(*it, this);
			}

			void readSwitchSource(float *buffer, uint32 index, uint32 requestFrames)
			{
				switch (dataType)
				{
				case DataTypeEnum::Raw:
					CAGE_ASSERT((index + requestFrames) * channels <= rawData.size());
					detail::memcpy(buffer, rawData.data() + index * channels, requestFrames * channels * sizeof(float));
					break;
				case DataTypeEnum::Vorbis:
					vorbisData.read(buffer, index, requestFrames);
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid data type");
				}
			}

			void readCycle(float *buffer, sint64 index, uint32 requestFrames)
			{
				if (index < 0)
					index += (-index / frames + 1) * frames;
				while (requestFrames)
				{
					if (index >= frames)
						index -= (index / frames) * frames;
					uint32 r = min(requestFrames, frames - numeric_cast<uint32>(index));
					readSwitchSource(buffer, numeric_cast<uint32>(index), r);
					buffer += r * channels;
					requestFrames -= r;
				}
			}

			void readCheckMargins(const SoundDataBuffer &buf)
			{
				// prepare a buffer
				const uint32 requestFrames = buf.frames * sampleRate / buf.sampleRate + 1;
				temporaryData1.resize(requestFrames * channels);

				{ // fill in the buffer
					sint64 index = buf.time * sampleRate / 1000000;
					float *buffer = temporaryData1.data();
					uint32 requestLeft = requestFrames;

					if (index < 0)
					{ // before start
						uint32 r = numeric_cast<uint32>(min(-index, (sint64)requestLeft));
						if (repeatBeforeStart)
							readCycle(buffer, index, r);
						else
							detail::memset(buffer, 0, r * channels * sizeof(float));
						buffer += channels * r;
						requestLeft -= r;
						index += r;
					}

					if (index < frames && requestLeft)
					{ // inside
						uint32 r = numeric_cast<uint32>(min((sint64)frames - index, (sint64)requestLeft));
						readSwitchSource(buffer, numeric_cast<uint32>(index), r);
						buffer += channels * r;
						requestLeft -= r;
						index += r;
					}

					if (index >= frames && requestLeft)
					{ // after end
						uint32 r = requestLeft;
						if (repeatAfterEnd)
							readCycle(buffer, index, r);
						else
							detail::memset(buffer, 0, r * channels * sizeof(float));
						buffer += channels * r;
						requestLeft -= r;
						index += r;
					}

					CAGE_ASSERT(buffer == temporaryData1.data() + temporaryData1.size());
					CAGE_ASSERT(requestLeft == 0);
				}

				// transfer the buffer
				temporaryData3.resize(buf.channels * buf.frames);
				transfer(temporaryData2, temporaryData1.data(), temporaryData3.data(), channels, buf.channels, requestFrames, buf.frames, sampleRate, buf.sampleRate);
				for (uint32 i = 0, e = buf.channels * buf.frames; i != e; i++)
					buf.buffer[i] += temporaryData3[i];
			}

			void synthesizeTone(const SoundDataBuffer &buf)
			{
				static constexpr double twoPi = 3.14159265358979323846264338327950288419716939937510 * 2;
				double mul = pitch * twoPi;
				for (uint32 sampleIndex = 0; sampleIndex < buf.frames; sampleIndex++)
				{
					double angle = buf.time * mul / 1e6 + sampleIndex * mul / buf.sampleRate;
					angle -= (sint64)(angle / twoPi) * twoPi; // modulo before conversion to float
					real sample = sin(rads(angle));
					for (uint32 ch = 0; ch < buf.channels; ch++)
						buf.buffer[sampleIndex * buf.channels + ch] += sample.value;
				}
			}

			void execute(const SoundDataBuffer &buf)
			{
				switch (dataType)
				{
				case DataTypeEnum::None:
					break;
				case DataTypeEnum::Raw:
				case DataTypeEnum::Vorbis:
					readCheckMargins(buf);
					break;
				case DataTypeEnum::Tone:
					synthesizeTone(buf);
					break;
				case DataTypeEnum::Noise:
					for (uint32 sampleIndex = 0; sampleIndex < buf.frames; sampleIndex++)
					{
						real sample = randomChance() * 2 - 1;
						for (uint32 ch = 0; ch < buf.channels; ch++)
							buf.buffer[sampleIndex * buf.channels + ch] += sample.value;
					}
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid source data type");
				}
			}

			void busDestroyed(MixingBus *bus)
			{
				CAGE_ASSERT(outputs.find(bus) != outputs.end());
				outputs.erase(bus);
			}

			void clearAllBuffers()
			{
				dataType = DataTypeEnum::None;
				channels = 0;
				frames = 0;
				sampleRate = 0;
				std::vector<float>().swap(rawData);
				std::vector<float>().swap(temporaryData1);
				std::vector<float>().swap(temporaryData2);
				std::vector<float>().swap(temporaryData3);
				vorbisData.clear();
			}
		};
	}

	void SoundSource::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
	}

	void SoundSource::setDataNone()
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		impl->clearAllBuffers();
	}

	void SoundSource::setDataRaw(uint32 channels, uint32 frames, uint32 sampleRate, PointerRange<const float> data)
	{
		CAGE_ASSERT(frames > 0);
		CAGE_ASSERT(channels > 0);
		CAGE_ASSERT(sampleRate > 0);
		CAGE_ASSERT(!data.empty());
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = DataTypeEnum::Raw;
		impl->channels = channels;
		impl->frames = frames;
		impl->sampleRate = sampleRate;
		impl->rawData.resize(channels * frames);
		detail::memcpy(&impl->rawData[0], data.data(), channels * frames * sizeof(float));
	}

	void SoundSource::setDataVorbis(PointerRange<const char> buffer)
	{
		CAGE_ASSERT(buffer.size() > 0);
		CAGE_ASSERT(!buffer.empty());
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = DataTypeEnum::Vorbis;
		impl->vorbisData.init(buffer.data(), buffer.size());
		impl->vorbisData.decode(impl->channels, impl->frames, impl->sampleRate, nullptr);
	}

	void SoundSource::setDataTone(uint32 pitch)
	{
		CAGE_ASSERT(pitch > 0);
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = DataTypeEnum::Tone;
		impl->pitch = pitch;
	}

	void SoundSource::setDataNoise()
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = DataTypeEnum::Noise;
	}

	void SoundSource::setDataRepeat(bool repeatBeforeStart, bool repeatAfterEnd)
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		impl->repeatBeforeStart = repeatBeforeStart;
		impl->repeatAfterEnd = repeatAfterEnd;
	}

	void SoundSource::addOutput(MixingBus *bus)
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		busAddInput(bus, impl);
		impl->outputs.insert(bus);
	}

	void SoundSource::removeOutput(MixingBus *bus)
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		busRemoveInput(bus, impl);
		impl->outputs.erase(bus);
	}

	uint64 SoundSource::getDuration() const
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		return (uint64)1000000 * impl->frames / impl->sampleRate;
	}

	uint32 SoundSource::getChannels() const
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		return impl->channels;
	}

	uint32 SoundSource::getSampleRate() const
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		return impl->sampleRate;
	}

	Holder<SoundSource> newSoundSource()
	{
		return detail::systemArena().createImpl<SoundSource, SoundSourceImpl>();
	}
}
