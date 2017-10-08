#include <set>
#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include "vorbisDecoder.h"
#include "private.h"
#include "utilities.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>

namespace cage
{
	namespace
	{
		enum dataTypeEnum
		{
			dtNone,
			dtRaw,
			dtVorbis,
			dtTone,
			dtNoise,
		};

		class soundSourceImpl : public sourceClass, public busInterfaceStruct
		{
		public:
			std::set<busClass*, std::less<busClass*>, memoryArenaStd<> > outputs;
			std::vector<float> rawData;
			std::vector<float> temporaryData1;
			std::vector<float> temporaryData2;
			std::vector<float> temporaryData3;
			uint32 channels;
			uint32 frames;
			uint32 sampleRate;
			uint32 pitch;
			dataTypeEnum dataType;
			bool repeatBeforeStart;
			bool repeatAfterEnd;

			soundPrivat::vorbisDataStruct vorbisData;

			soundSourceImpl(soundContextClass *context) :
				busInterfaceStruct(delegate<void(busClass*)>().bind<soundSourceImpl, &soundSourceImpl::busDestroyed>(this), delegate<void(const soundDataBufferStruct&)>().bind<soundSourceImpl, &soundSourceImpl::execute>(this)),
				outputs(linksArenaFromContext(context)),
				channels(0), frames(0), sampleRate(0), pitch(0), dataType(dtNone), repeatBeforeStart(false), repeatAfterEnd(false)
			{}

			~soundSourceImpl()
			{
				for (auto it = outputs.begin(), et = outputs.end(); it != et; it++)
					busRemoveInput(*it, this);
			}

			void readSwitchSource(float *buffer, uint32 index, uint32 requestFrames)
			{
				switch (dataType)
				{
				case dtRaw:
					CAGE_ASSERT_RUNTIME((index + requestFrames) * channels <= rawData.size());
					detail::memcpy(buffer, rawData.data() + index * channels, requestFrames * channels * sizeof(float));
					break;
				case dtVorbis:
					vorbisData.read(buffer, index, requestFrames);
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid data type");
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

			void readCheckMargins(const soundDataBufferStruct &buf)
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
						const uint32 r = min(numeric_cast<uint32>(-index), requestLeft);
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
						const uint32 r = min(frames - numeric_cast<uint32>(index), requestLeft);
						readSwitchSource(buffer, numeric_cast<uint32>(index), r);
						buffer += channels * r;
						requestLeft -= r;
						index += r;
					}

					if (index >= frames && requestLeft)
					{ // after end
						const uint32 r = requestLeft;
						if (repeatAfterEnd)
							readCycle(buffer, index, r);
						else
							detail::memset(buffer, 0, r * channels * sizeof(float));
						buffer += channels * r;
						requestLeft -= r;
						index += r;
					}

					CAGE_ASSERT_RUNTIME(buffer == temporaryData1.data() + temporaryData1.size());
					CAGE_ASSERT_RUNTIME(requestLeft == 0);
				}

				// transfer the buffer
				temporaryData3.resize(buf.channels * buf.frames);
				transfer(temporaryData2, temporaryData1.data(), temporaryData3.data(), channels, buf.channels, requestFrames, buf.frames, sampleRate, buf.sampleRate);
				for (uint32 i = 0, e = buf.channels * buf.frames; i != e; i++)
					buf.buffer[i] += temporaryData3[i];
			}

			void synthesizeTone(const soundDataBufferStruct &buf)
			{
				static const double twoPi = 3.14159265358979323846264338327950288419716939937510 * 2;
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

			void execute(const soundDataBufferStruct &buf)
			{
				switch (dataType)
				{
				case dtNone:
					break;
				case dtRaw:
				case dtVorbis:
					readCheckMargins(buf);
					break;
				case dtTone:
					synthesizeTone(buf);
					break;
				case dtNoise:
					for (uint32 sampleIndex = 0; sampleIndex < buf.frames; sampleIndex++)
					{
						real sample = random() * 2 - 1;
						for (uint32 ch = 0; ch < buf.channels; ch++)
							buf.buffer[sampleIndex * buf.channels + ch] += sample.value;
					}
					break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid source data type");
				}
			}

			void busDestroyed(busClass *bus)
			{
				CAGE_ASSERT_RUNTIME(outputs.find(bus) != outputs.end());
				outputs.erase(bus);
			}

			void clearAllBuffers()
			{
				dataType = dtNone;
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

	void sourceClass::setDataNone()
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		impl->clearAllBuffers();
	}

	void sourceClass::setDataRaw(uint32 channels, uint32 frames, uint32 sampleRate, float *data)
	{
		CAGE_ASSERT_RUNTIME(frames > 0);
		CAGE_ASSERT_RUNTIME(channels > 0);
		CAGE_ASSERT_RUNTIME(sampleRate > 0);
		CAGE_ASSERT_RUNTIME(data != nullptr);
		soundSourceImpl *impl = (soundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = dtRaw;
		impl->channels = channels;
		impl->frames = frames;
		impl->sampleRate = sampleRate;
		impl->rawData.resize(channels * frames);
		detail::memcpy(&impl->rawData[0], data, channels * frames * sizeof(float));
	}

	void sourceClass::setDataVorbis(uintPtr size, void *buffer)
	{
		CAGE_ASSERT_RUNTIME(size > 0);
		CAGE_ASSERT_RUNTIME(buffer != nullptr);
		soundSourceImpl *impl = (soundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = dtVorbis;
		impl->vorbisData.init(buffer, size);
		impl->vorbisData.decode(impl->channels, impl->frames, impl->sampleRate, nullptr);
	}

	void sourceClass::setDataTone(uint32 pitch)
	{
		CAGE_ASSERT_RUNTIME(pitch > 0);
		soundSourceImpl *impl = (soundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = dtTone;
		impl->pitch = pitch;
	}

	void sourceClass::setDataNoise()
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		impl->clearAllBuffers();
		impl->dataType = dtNoise;
	}

	void sourceClass::setDataRepeat(bool repeatBeforeStart, bool repeatAfterEnd)
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		impl->repeatBeforeStart = repeatBeforeStart;
		impl->repeatAfterEnd = repeatAfterEnd;
	}

	void sourceClass::addOutput(busClass *bus)
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		busAddInput(bus, impl);
		impl->outputs.insert(bus);
	}

	void sourceClass::removeOutput(busClass *bus)
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		busRemoveInput(bus, impl);
		impl->outputs.erase(bus);
	}

	uint64 sourceClass::getDuration() const
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		return (uint64)1000000 * impl->frames / impl->sampleRate;
	}

	uint32 sourceClass::getChannels() const
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		return impl->channels;
	}

	uint32 sourceClass::getSampleRate() const
	{
		soundSourceImpl *impl = (soundSourceImpl*)this;
		return impl->sampleRate;
	}

	holder<sourceClass> newSource(soundContextClass *context)
	{
		return detail::systemArena().createImpl <sourceClass, soundSourceImpl>(context);
	}
}