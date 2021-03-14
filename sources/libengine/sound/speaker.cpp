#include <cage-core/concurrent.h>
#include <cage-engine/speaker.h>

#include "private.h"
#include "utilities.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <../src/cubeb_ringbuffer.h>

namespace cage
{
	namespace
	{
		class SpeakerImpl : public SpeakerOutput, public BusInterface
		{
		public:
			Holder<Speaker> spkr;
			MixingBus *inputBus = nullptr;
			uint64 lastTime = 0;
			SoundDataBuffer buffer;
			Holder<lock_free_audio_ring_buffer<float>> ring;

			SpeakerImpl(const SpeakerOutputCreateConfig &config, const string &name) :
				BusInterface(Delegate<void(MixingBus *)>().bind<SpeakerImpl, &SpeakerImpl::busDestroyed>(this), {})
			{
				SpeakerCreateConfig cfg;
				cfg.name = name;
				cfg.deviceId = config.deviceId;
				cfg.sampleRate = config.sampleRate;
				spkr = newSpeaker(cfg, Delegate<void(const SpeakerCallbackData &)>().bind<SpeakerImpl, &SpeakerImpl::callback>(this));

				buffer.channels = spkr->channels();
				buffer.sampleRate = spkr->sampleRate();

				ring = detail::systemArena().createHolder<lock_free_audio_ring_buffer<float>>(buffer.channels, buffer.sampleRate);

				spkr->start();
			}

			~SpeakerImpl()
			{
				setInput(nullptr);
			}

			void update(uint64 currentTime)
			{
				if (!inputBus)
					return;
				if (lastTime == 0)
				{
					lastTime = currentTime;
					return;
				}
				if (currentTime <= lastTime)
				{
					lastTime = currentTime;
					return;
				}

				const uint32 request = numeric_cast<uint32>(min(buffer.sampleRate * (currentTime - lastTime) / 1000000, (uint64)buffer.sampleRate));
				const uint64 elapsed = (uint64)request * 1000000 / buffer.sampleRate;
				lastTime += elapsed;
				const uint32 frames = min(request, numeric_cast<uint32>(ring->available_write()));
				if (frames == 0)
					return;

				buffer.resize(buffer.channels, frames);
				buffer.clear();
				buffer.time = lastTime;

				((BusInterface *)inputBus)->busExecuteDelegate(buffer);
				ring->enqueue(buffer.buffer, frames);
			}

			void callback(const SpeakerCallbackData &data)
			{
				if (data.frames == 0)
					return;
				const uint32 n = min(numeric_cast<uint32>(ring->available_read()), data.frames);
				uint32 r = ring->dequeue(data.buffer.data(), n);
				CAGE_ASSERT(r == n);
				float *buff = data.buffer.data() + r * data.channels;
				while (r < data.frames)
				{
					for (uint32 i = 0; i < buffer.channels; i++)
						*buff++ = 0;
					r++;
				}
			}

			void busDestroyed(MixingBus *bus)
			{
				CAGE_ASSERT(bus == inputBus);
				inputBus = nullptr;
			}
		};
	}

	uint32 SpeakerOutput::getChannels() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->spkr->channels();
	}

	uint32 SpeakerOutput::getSamplerate() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->spkr->sampleRate();
	}

	uint32 SpeakerOutput::getLatency() const
	{
		const SpeakerImpl *impl = (const SpeakerImpl *)this;
		return impl->spkr->latency();
	}

	void SpeakerOutput::setInput(MixingBus *bus)
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

	void SpeakerOutput::update(uint64 time)
	{
		SpeakerImpl *impl = (SpeakerImpl *)this;
		impl->update(time);
	}

	Holder<SpeakerOutput> newSpeakerOutput(const SpeakerOutputCreateConfig &config, const string &name)
	{
		return detail::systemArena().createImpl<SpeakerOutput, SpeakerImpl>(config, name);
	}
}
