#include <cage-core/concurrent.h>
#include <cage-engine/speaker.h>

#include "private.h"
#include "utilities.h"

namespace cage
{
	namespace
	{
		class SpeakerImpl : public SpeakerOutput, public BusInterface
		{
		public:
			Holder<Speaker> spkr;
			MixingBus *inputBus = nullptr;
			SoundDataBuffer buffer;

			SpeakerImpl(const SpeakerOutputCreateConfig &config, const string &name) :
				BusInterface(Delegate<void(MixingBus *)>().bind<SpeakerImpl, &SpeakerImpl::busDestroyed>(this), {})
			{
				SpeakerCreateConfig cfg;
				cfg.name = name;
				cfg.deviceId = config.deviceId;
				cfg.sampleRate = config.sampleRate;
				spkr = newSpeaker(cfg, Delegate<void(const SpeakerCallbackData &)>().bind<SpeakerImpl, &SpeakerImpl::callback>(this));
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
				spkr->update(currentTime);
			}

			void callback(const SpeakerCallbackData &data)
			{
				buffer.resize(data.channels, data.frames);
				buffer.clear();
				buffer.sampleRate = data.sampleRate;
				buffer.time = data.time;
				((BusInterface *)inputBus)->busExecuteDelegate(buffer);
				detail::memcpy(data.buffer.data(), buffer.buffer, sizeof(float) * data.channels * data.frames);
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
