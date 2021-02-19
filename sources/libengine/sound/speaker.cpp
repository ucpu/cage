#define NOMINMAX

#include <cage-core/concurrent.h>
#include <cage-core/string.h>

#include "private.h"
#include "utilities.h"

#include <cubeb/cubeb.h>
#include <../src/cubeb_ringbuffer.h>

namespace cage
{
	namespace
	{
		long dataCallbackFree(cubeb_stream *stream, void *user_ptr, void const *input_buffer, void *output_buffer, long nframes);
		void stateCallbackFree(cubeb_stream *stream, void *user_ptr, cubeb_state state);

		class SpeakerImpl : public Speaker, public BusInterface
		{
		public:
			const string name;
			string deviceId;
			SoundContext *const context = nullptr;
			cubeb_stream *stream = nullptr;
			MixingBus *inputBus = nullptr;
			uint64 lastTime = 0;
			SoundDataBuffer buffer;
			Holder<lock_free_audio_ring_buffer<float>> ring;

			SpeakerImpl(SoundContext *context, const SpeakerCreateConfig &config, const string &name_) :
				BusInterface(Delegate<void(MixingBus *)>().bind<SpeakerImpl, &SpeakerImpl::busDestroyed>(this), {}),
				name(replace(name_, ":", "_")), context(context)
			{
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "creating speaker, name: '" + name + "'");
				cubeb *snd = soundioFromContext(context);

				cubeb_stream_params params = {};
				params.format = CUBEB_SAMPLE_FLOAT32NE;
				params.channels = 2;
				if (config.sampleRate)
					params.rate = config.sampleRate;
				else
					checkSoundIoError(cubeb_get_preferred_sample_rate(snd, &params.rate));
				uint32 latency = 0;
				checkSoundIoError(cubeb_get_min_latency(snd, &params, &latency));

				cubeb_devid device = nullptr;
				if (!config.deviceId.empty())
				{
					cubeb_device_collection collection = {};
					checkSoundIoError(cubeb_enumerate_devices(snd, CUBEB_DEVICE_TYPE_OUTPUT, &collection));
					for (uint32 index = 0; index < collection.count; index++)
					{
						const cubeb_device_info &d = collection.device[index];
						if (d.device_id == config.deviceId)
						{
							device = d.devid;
							deviceId = d.device_id;
						}
					}
					cubeb_device_collection_destroy(snd, &collection);
					if (!device)
						CAGE_THROW_ERROR(Exception, "invalid sound device id");
				}

				checkSoundIoError(cubeb_stream_init(snd, &stream, name.c_str(), nullptr, nullptr, device, &params, latency, &dataCallbackFree, &stateCallbackFree, this));

				buffer.channels = params.channels;
				buffer.sampleRate = params.rate;

				ring = detail::systemArena().createHolder<lock_free_audio_ring_buffer<float>>(buffer.channels, buffer.sampleRate / 2);

				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "using device: '" + getDeviceId() + "'");
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "using channels: " + buffer.channels);
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "using sample rate: " + buffer.sampleRate);
				CAGE_LOG(SeverityEnum::Info, "sound", stringizer() + "using latency: " + latency);

				checkSoundIoError(cubeb_stream_start(stream));
			}

			~SpeakerImpl()
			{
				if (stream)
				{
					cubeb_stream_stop(stream);
					cubeb_stream_destroy(stream);
				}
				setInput(nullptr);
			}

			void update(uint64 currentTime)
			{
				updateImpl(currentTime);
				lastTime = currentTime;
			}

			void updateImpl(uint64 currentTime)
			{
				if (!inputBus)
					return;
				if (lastTime == 0)
					return;
				if (currentTime <= lastTime)
					return;

				const uint64 frames64 = buffer.sampleRate * (currentTime - lastTime) / 1000000;
				const uint32 frames = numeric_cast<uint32>(min(numeric_cast<uint64>(ring->available_write()), frames64));
				if (frames == 0)
					return;

				buffer.resize(buffer.channels, frames);
				buffer.clear();
				buffer.time = currentTime;

				((BusInterface *)inputBus)->busExecuteDelegate(buffer);
				ring->enqueue(buffer.buffer, frames);
			}

			uint32 dataCallback(float *output_buffer, uint32 nframes)
			{
				const uint32 n = min(numeric_cast<uint32>(ring->available_read()), nframes);
				uint32 r = ring->dequeue(output_buffer, n);
				CAGE_ASSERT(r == n);
				output_buffer += r * buffer.channels;
				while (r < nframes)
				{
					for (uint32 i = 0; i < buffer.channels; i++)
						*output_buffer++ = 0;
					r++;
				}
				return r;
			}

			void stateCallback(cubeb_state state)
			{
				// nothing
			}

			void busDestroyed(MixingBus *bus)
			{
				CAGE_ASSERT(bus == inputBus);
				inputBus = nullptr;
			}
		};

		long dataCallbackFree(cubeb_stream *stream, void *user_ptr, void const *input_buffer, void *output_buffer, long nframes)
		{
			return numeric_cast<uint32>(((SpeakerImpl *)user_ptr)->dataCallback((float *)output_buffer, numeric_cast<uint32>(nframes)));
		}

		void stateCallbackFree(cubeb_stream *stream, void *user_ptr, cubeb_state state)
		{
			((SpeakerImpl *)user_ptr)->stateCallback(state);
		}
	}

	string Speaker::getStreamName() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->name;
	}

	string Speaker::getDeviceId() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->deviceId;
	}

	uint32 Speaker::getChannels() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->buffer.channels;
	}

	uint32 Speaker::getSamplerate() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->buffer.sampleRate;
	}

	uint32 Speaker::getLatency() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		uint32 res = 0;
		checkSoundIoError(cubeb_stream_get_latency(impl->stream, &res));
		return res;
	}

	void Speaker::setInput(MixingBus *bus)
	{
		SpeakerImpl *impl = (SpeakerImpl *)this;
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

	void Speaker::update(uint64 time)
	{
		SpeakerImpl *impl = (SpeakerImpl *)this;
		impl->update(time);
	}

	Holder<Speaker> newSpeakerOutput(SoundContext *context, const SpeakerCreateConfig &config, const string &name)
	{
		return detail::systemArena().createImpl<Speaker, SpeakerImpl>(context, config, name);
	}
}
