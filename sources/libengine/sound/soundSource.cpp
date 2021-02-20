#include "private.h"
#include "utilities.h"

#include <cage-core/polytone.h>

#include <set>

namespace cage
{
	namespace
	{
		enum class DataTypeEnum : uint32
		{
			None = 0,
			Poly,
			Tone,
			Noise,
		};

		class SoundSourceImpl : public SoundSource, public BusInterface
		{
		public:
			Holder<PolytoneStream> poly;
			std::set<MixingBus *> outputs;
			std::vector<float> temporaryData1;
			std::vector<float> temporaryData2;
			std::vector<float> temporaryData3;
			uint32 pitch = 0;
			DataTypeEnum dataType = DataTypeEnum::None;
			bool repeatBeforeStart = false;
			bool repeatAfterEnd = false;

			SoundSourceImpl() : BusInterface(Delegate<void(MixingBus*)>().bind<SoundSourceImpl, &SoundSourceImpl::busDestroyed>(this), Delegate<void(const SoundDataBuffer&)>().bind<SoundSourceImpl, &SoundSourceImpl::execute>(this))
			{}

			~SoundSourceImpl()
			{
				for (auto it = outputs.begin(), et = outputs.end(); it != et; it++)
					busRemoveInput(*it, this);
			}

			void readSwitchSource(float *buffer, uint64 index, uint64 requestFrames)
			{
				poly->decode(index, { buffer, buffer + requestFrames * poly->source()->channels() });
			}

			void readCycle(float *buffer, sint64 index, uint64 requestFrames)
			{
				const uint32 channels = poly->source()->channels();
				const uint64 frames = poly->source()->frames();

				if (index < 0)
					index += (-index / frames + 1) * frames;
				while (requestFrames)
				{
					if (index >= (sint64)frames)
						index -= (index / frames) * frames;
					uint64 r = min(requestFrames, frames - index);
					readSwitchSource(buffer, index, r);
					buffer += r * channels;
					requestFrames -= r;
				}
			}

			void readCheckMargins(const SoundDataBuffer &buf)
			{
				const uint32 channels = poly->source()->channels();
				const uint32 sampleRate = poly->source()->sampleRate();
				const uint64 frames = poly->source()->frames();

				// prepare a buffer
				const uint64 requestFrames = buf.frames * sampleRate / buf.sampleRate + 1;
				temporaryData1.resize(requestFrames * channels);

				{ // fill in the buffer
					sint64 index = buf.time * sampleRate / 1000000;
					float *buffer = temporaryData1.data();
					uint64 requestLeft = requestFrames;

					if (index < 0)
					{ // before start
						uint64 r = min(numeric_cast<uint64>(-index), requestLeft);
						if (repeatBeforeStart)
							readCycle(buffer, index, r);
						else
							detail::memset(buffer, 0, r * channels * sizeof(float));
						buffer += channels * r;
						requestLeft -= r;
						index += r;
					}

					if (index < (sint64)frames && requestLeft)
					{ // inside
						uint64 r = min(frames - index, requestLeft);
						readSwitchSource(buffer, index, r);
						buffer += channels * r;
						requestLeft -= r;
						index += r;
					}

					if (index >= (sint64)frames && requestLeft)
					{ // after end
						uint64 r = requestLeft;
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
				transfer(temporaryData2, temporaryData1.data(), temporaryData3.data(), channels, buf.channels, numeric_cast<uint32>(requestFrames), buf.frames, sampleRate, buf.sampleRate);
				for (uint64 i = 0, e = buf.channels * buf.frames; i != e; i++)
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

			void synthesizeNoise(const SoundDataBuffer &buf)
			{
				for (uint32 sampleIndex = 0; sampleIndex < buf.frames; sampleIndex++)
				{
					real sample = randomChance() * 2 - 1;
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
				case DataTypeEnum::Poly:
					readCheckMargins(buf);
					break;
				case DataTypeEnum::Tone:
					synthesizeTone(buf);
					break;
				case DataTypeEnum::Noise:
					synthesizeNoise(buf);
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
				poly.clear();
				std::vector<float>().swap(temporaryData1);
				std::vector<float>().swap(temporaryData2);
				std::vector<float>().swap(temporaryData3);
			}
		};
	}

	void SoundSource::setDebugName(const string &name)
	{
#ifdef CAGE_DEBUG
		debugName = name;
#endif // CAGE_DEBUG
	}

	void SoundSource::clear()
	{
		SoundSourceImpl *impl = (SoundSourceImpl*)this;
		impl->clearAllBuffers();
	}

	void SoundSource::setData(Holder<Polytone> &&poly)
	{
		SoundSourceImpl *impl = (SoundSourceImpl *)this;
		impl->clearAllBuffers();
		impl->dataType = DataTypeEnum::Poly;
		impl->poly = newPolytoneStream(templates::move(poly));
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

	uint64 SoundSource::getDuration() const
	{
		const SoundSourceImpl *impl = (const SoundSourceImpl *)this;
		if (impl->dataType == DataTypeEnum::Poly)
			return impl->poly->source()->duration();
		return 0;
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

	Holder<SoundSource> newSoundSource()
	{
		return detail::systemArena().createImpl<SoundSource, SoundSourceImpl>();
	}
}
