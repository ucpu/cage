#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>
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
		const uint32 channelIndex(uint32 channels, SoundIoChannelId id)
		{
			CAGE_ASSERT_RUNTIME(channels > 0);
			switch (id)
			{
			case SoundIoChannelIdFrontLeft:
			case SoundIoChannelIdFrontLeftCenter:
			case SoundIoChannelIdTopFrontLeft:
				return 0;
			case SoundIoChannelIdFrontRight:
			case SoundIoChannelIdFrontRightCenter:
			case SoundIoChannelIdTopFrontRight:
				switch (channels)
				{
				case 1: return 0;
				case 2: return 1;
				case 3: return 2;
				case 4: return 1;
				case 5: return 2;
				case 6: return 2;
				case 7: return 2;
				default: return 2;
				}
			case SoundIoChannelIdFrontCenter:
			case SoundIoChannelIdTopCenter:
			case SoundIoChannelIdTopFrontCenter:
				switch (channels)
				{
				case 1: return 0;
				case 2: return -1;
				case 3: return 1;
				case 4: return -1;
				case 5: return 1;
				case 6: return 1;
				case 7: return 1;
				default: return 1;
				}
			case SoundIoChannelIdLfe:
				switch (channels)
				{
				case 1: return -1;
				case 2: return -1;
				case 3: return -1;
				case 4: return -1;
				case 5: return -1;
				case 6: return 5;
				case 7: return 6;
				default: return 7;
				}
			case SoundIoChannelIdBackLeft:
			case SoundIoChannelIdTopBackLeft:
				switch (channels)
				{
				case 1: return -1;
				case 2: return -1;
				case 3: return -1;
				case 4: return 2;
				case 5: return 3;
				case 6: return 3;
				case 7: return -1;
				default: return 5;
				}
			case SoundIoChannelIdBackRight:
			case SoundIoChannelIdTopBackRight:
				switch (channels)
				{
				case 1: return -1;
				case 2: return -1;
				case 3: return -1;
				case 4: return 3;
				case 5: return 4;
				case 6: return 4;
				case 7: return -1;
				default: return 6;
				}
			case SoundIoChannelIdBackCenter:
			case SoundIoChannelIdTopBackCenter:
				switch (channels)
				{
				case 1: return -1;
				case 2: return -1;
				case 3: return -1;
				case 4: return -1;
				case 5: return -1;
				case 6: return -1;
				case 7: return 5;
				default: return -1;
				}
			case SoundIoChannelIdSideLeft:
				switch (channels)
				{
				case 1: return -1;
				case 2: return -1;
				case 3: return -1;
				case 4: return -1;
				case 5: return -1;
				case 6: return -1;
				case 7: return 3;
				default: return 3;
				}
			case SoundIoChannelIdSideRight:
				switch (channels)
				{
				case 1: return -1;
				case 2: return -1;
				case 3: return -1;
				case 4: return -1;
				case 5: return -1;
				case 6: return -1;
				case 7: return 4;
				default: return 4;
				}
			default:
				return -1;
			}
		}

		void remapChannels(uint32 channels, const SoundIoChannelId *device, uint32 *mapping)
		{
			for (uint32 i = 0; i < channels; i++)
				mapping[i] = channelIndex(channels, device[i]);
#ifdef CAGE_ASSERT_ENABLED
			for (uint32 i = 0; i < channels; i++)
				if (mapping[i] != -1)
					CAGE_ASSERT_RUNTIME(mapping[i] < min(8u, channels), mapping[i], device[i], channels, i);
#endif // CAGE_ASSERT_ENABLED
			string sm, sd;
			for (uint32 i = 0; i < channels; i++)
			{
				sm += string(mapping[i]).fill(4);
				sd += string(device[i]).fill(4);
			}
			CAGE_LOG_CONTINUE(severityEnum::Note, "sound", string() + "device channel ids:  " + sd);
			CAGE_LOG_CONTINUE(severityEnum::Note, "sound", string() + "mapping channel ids: " + sm);
		}

		void writeCallbackHelper(SoundIoOutStream *stream, int frameCountMin, int frameCountMax);
		void underflowCallbackHelper(SoundIoOutStream *stream);
		typedef void(*convertCallbackType)(float source, void *target);

		class soundSpeakerImpl : public speakerClass, public busInterfaceStruct
		{
		public:
			string name;
			soundContextClass *context;
			SoundIoDevice *device;
			SoundIoOutStream *stream;
			busClass *inputBus;
			convertCallbackType convertCallback;
			uint32 errorCountUnderflow;
			uint32 errorCountExceptions;
			uint32 errorCountNoData;
			std::vector<uint32> channelMapping;

			struct dataStruct
			{
				struct ringBufferStruct
				{
					ringBufferStruct() : buffer(nullptr)
					{}

					~ringBufferStruct()
					{
						soundio_ring_buffer_destroy(buffer);
					}

					void init(SoundIo *context, uint32 capacity)
					{
						buffer = soundio_ring_buffer_create(context, capacity * sizeof(float));
						if (!buffer)
							checkSoundIoError(SoundIoErrorNoMem);
					}

					const uint32 space() const
					{
						return soundio_ring_buffer_free_count(buffer) / sizeof(float);
					}

					const uint32 available() const
					{
						return soundio_ring_buffer_fill_count(buffer) / sizeof(float);
					}

					void write(const float *data, uint32 count)
					{
						while (space() < count)
							threadSleep(1000);

						float *wp = (float*)soundio_ring_buffer_write_ptr(buffer);
						detail::memcpy(wp, data, count * sizeof(float));
						soundio_ring_buffer_advance_write_ptr(buffer, count * sizeof(float));
					}

					SoundIoRingBuffer *buffer;
				};

				soundDataBufferStruct buffer;
				ringBufferStruct ring;
				bool first;
				sint64 time;

				dataStruct() : first(true) {}
				void init(SoundIo *context, uint32 channels, uint32 sampleRate)
				{
					CAGE_ASSERT_RUNTIME(channels <= 8);
					buffer.channels = channels;
					buffer.sampleRate = sampleRate;
					ring.init(context, channels * sampleRate * 2);
				}
			} data;

			soundSpeakerImpl(soundContextClass *context, const speakerCreateConfig &config, string name) :
				busInterfaceStruct(delegate<void(busClass*)>().bind<soundSpeakerImpl, &soundSpeakerImpl::busDestroyed>(this), delegate<void(const soundDataBufferStruct&)>()),
				name(name.replace(":", "_")), context(context),
				device(nullptr), stream(nullptr), inputBus(nullptr), convertCallback(nullptr),
				errorCountUnderflow(0), errorCountExceptions(0), errorCountNoData(0)
			{
				CAGE_LOG(severityEnum::Info, "sound", string() + "creating speaker, name: '" + name + "'");

				for (uint32 i = 0; i < sizeof(channelVolumes) / sizeof(channelVolumes[0]); i++)
					channelVolumes[i] = 1;

				SoundIo *sio = soundioFromContext(context);

				int deviceIndex = soundio_default_output_device_index(sio);
				{
					int deviceCount = soundio_output_device_count(sio);
					for (int i = 0; i < deviceCount; i++)
					{
						SoundIoDevice *dev = soundio_get_output_device(sio, i);
						if (dev->is_raw == config.deviceRaw && string(dev->id) == config.deviceId)
						{
							deviceIndex = i;
							soundio_device_unref(dev);
							break;
						}
						soundio_device_unref(dev);
					}
				}

				device = soundio_get_output_device(sio, deviceIndex);
				if (!device)
					CAGE_THROW_ERROR(soundException, "failed to find a speaker device", 0);
				if (device->probe_error)
					CAGE_THROW_ERROR(soundException, "speaker device probe error", device->probe_error);

				stream = soundio_outstream_create(device);
				if (!stream)
					CAGE_THROW_ERROR(soundException, "failed to create a speaker stream", 0);

				stream->userdata = this;
				stream->write_callback = &writeCallbackHelper;
				stream->underflow_callback = &underflowCallbackHelper;
				stream->name = this->name.c_str();
				if (config.sampleRate)
					stream->sample_rate = config.sampleRate;

				if (soundio_device_supports_format(device, stream->format = SoundIoFormatFloat32NE))
				{
					convertCallback = &soundPrivat::convertSingle<float, float>;
					CAGE_LOG_CONTINUE(severityEnum::Note, "sound", "speaker uses 32 bit float samples");
				}
				else if (soundio_device_supports_format(device, stream->format = SoundIoFormatS32NE))
				{
					convertCallback = &soundPrivat::convertSingle<float, sint32>;
					CAGE_LOG_CONTINUE(severityEnum::Note, "sound", "speaker uses 32 bit integer samples");
				}
				else if (soundio_device_supports_format(device, stream->format = SoundIoFormatS16NE))
				{
					convertCallback = &soundPrivat::convertSingle<float, sint16>;
					CAGE_LOG_CONTINUE(severityEnum::Note, "sound", "speaker uses 16 bit integer samples");
				}
				else if (soundio_device_supports_format(device, stream->format = SoundIoFormatFloat64NE))
				{
					convertCallback = &soundPrivat::convertSingle<float, double>;
					CAGE_LOG_CONTINUE(severityEnum::Note, "sound", "speaker uses 64 bit double samples");
				}
				else
					CAGE_THROW_ERROR(soundException, "no supported format available", 0);

				checkSoundIoError(soundio_outstream_open(stream));
				CAGE_LOG_CONTINUE(severityEnum::Note, "sound", string() + "speaker uses " + stream->layout.channel_count + " channels and " + stream->sample_rate + " samples per second per channel");
				CAGE_LOG_CONTINUE(severityEnum::Note, "sound", string() + "device id: '" + device->id + "'");
				CAGE_LOG_CONTINUE(severityEnum::Note, "sound", string() + "device name: '" + device->name + "'");
				channelMapping.resize(stream->layout.channel_count);
				remapChannels(stream->layout.channel_count, stream->layout.channels, channelMapping.data());
				data.init(sio, min(stream->layout.channel_count, 8), stream->sample_rate);
				checkSoundIoError(soundio_outstream_start(stream));
			}

			~soundSpeakerImpl()
			{
				setInput(nullptr);
				soundio_outstream_destroy(stream);
				soundio_device_unref(device);
				if (errorCountUnderflow || errorCountExceptions || errorCountNoData)
					CAGE_LOG(severityEnum::Warning, "sound", string() + "there has been " + errorCountUnderflow + " underflows, " + errorCountExceptions + " exceptions and " + errorCountNoData + " times no data in audio speaker: '" + name + "'");
			}

			void update(uint64 currentTime)
			{
				if (!inputBus)
					return;

				if (data.first)
				{
					data.time = currentTime;
					data.first = false;
					return;
				}

				uint32 request = numeric_cast<uint32>((currentTime - data.time) * stream->sample_rate / 1000000);
				if (request == 0)
					return;

				uint32 marginSamples = 20;
				uint32 neededFrames = min(request, data.buffer.sampleRate);
				uint32 requestFrames = neededFrames + marginSamples;

				data.buffer.resize(data.buffer.channels, requestFrames);
				data.buffer.clear();
				data.buffer.time = data.time - (sint32)marginSamples / 2 * 1000000 / (sint32)data.buffer.sampleRate;
				data.time += (uint64)request * 1000000 / stream->sample_rate;
				((busInterfaceStruct*)inputBus)->busExecuteDelegate(data.buffer);
				data.ring.write(data.buffer.buffer + data.buffer.channels * marginSamples / 2, data.buffer.channels * neededFrames);
			}

			void writeCallback(uint32 frameCountMin, uint32 frameCountMax)
			{
				uint32 avail = data.ring.available() / stream->layout.channel_count;
				uint32 needToWrite = clamp(max(avail, 512u), frameCountMin, frameCountMax);
				while (needToWrite)
				{
					uint32 frames = needToWrite;
					SoundIoChannelArea *areas = nullptr;
					checkSoundIoError(soundio_outstream_begin_write(stream, &areas, (int*)&frames));
					if (frames == 0)
						break;
					uint32 a = min(avail, frames);
					uint32 b = frames - a;
					if (a)
					{
						const float *rp = (float*)soundio_ring_buffer_read_ptr(data.ring.buffer);
						for (uint32 f = 0; f < a; f++)
						{
							const float *frame = rp;
							for (uint32 ch = 0; ch < numeric_cast<uint32>(stream->layout.channel_count); ch++)
							{
								if (channelMapping[ch] == -1)
									convertCallback(0, areas[ch].ptr);
								else
									convertCallback(frame[channelMapping[ch]] * (ch < sizeof(channelVolumes) / sizeof(channelVolumes[0]) ? channelVolumes[ch] : 0.f), areas[ch].ptr);
								areas[ch].ptr += areas[ch].step;
							}
							rp += data.buffer.channels;
						}
						soundio_ring_buffer_advance_read_ptr(data.ring.buffer, a * stream->layout.channel_count * sizeof(float));
					}
					if (b)
					{
						for (uint32 f = 0; f < b; f++)
						{
							for (uint32 ch = 0; ch < numeric_cast<uint32>(stream->layout.channel_count); ch++)
							{
								convertCallback(0, areas[ch].ptr);
								areas[ch].ptr += areas[ch].step;
							}
						}
					}
					checkSoundIoError(soundio_outstream_end_write(stream));
					needToWrite -= frames;
					avail -= a;
				}
			}

			void busDestroyed(busClass *bus)
			{
				CAGE_ASSERT_RUNTIME(bus == inputBus);
				inputBus = nullptr;
			}
		};

		void writeCallbackHelper(SoundIoOutStream *stream, int frameCountMin, int frameCountMax)
		{
			try
			{
				((soundSpeakerImpl*)stream->userdata)->writeCallback(frameCountMin, frameCountMax);
			}
			catch (...)
			{
				((soundSpeakerImpl*)stream->userdata)->errorCountExceptions++;
			}
		}

		void underflowCallbackHelper(SoundIoOutStream *stream)
		{}
	}

	string speakerClass::getStreamName() const
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		return impl->name;
	}

	string speakerClass::getDeviceId() const
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		return impl->device->id;
	}

	string speakerClass::getDeviceName() const
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		return impl->device->name;
	}

	bool speakerClass::getDeviceRaw() const
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		return impl->device->is_raw;
	}

	string speakerClass::getLayoutName() const
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		return impl->stream->layout.name;
	}

	uint32 speakerClass::getChannelsCount() const
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		return impl->stream->layout.channel_count;
	}

	uint32 speakerClass::getOutputSampleRate() const
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		return impl->stream->sample_rate;
	}

	void speakerClass::setInput(busClass *bus)
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		if (impl->inputBus)
		{
			busRemoveOutput(impl->inputBus, impl);
			impl->inputBus = nullptr;
		}
		if (bus)
		{
			busAddOutput(bus, impl);
			impl->inputBus = bus;
		}
	}

	void speakerClass::update(uint64 time)
	{
		soundSpeakerImpl *impl = (soundSpeakerImpl*)this;
		impl->update(time);
	}

	speakerCreateConfig::speakerCreateConfig() : sampleRate(0), deviceRaw(false)
	{}

	holder<speakerClass> newSpeaker(soundContextClass *context, const speakerCreateConfig &config, string name)
	{
		return detail::systemArena().createImpl <speakerClass, soundSpeakerImpl>(context, config, name);
	}
}
