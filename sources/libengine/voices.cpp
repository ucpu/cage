#include <cage-engine/voices.h>

#include <plf_colony.h>

#include <vector>

namespace cage
{
	namespace
	{
		struct VoiceImpl : public VoiceProperties
		{

		};

		class VoicesMixerImpl : public VoicesMixer
		{
		public:
			VoicesMixerImpl(const VoicesMixerCreateConfig &config)
			{}

			ListenerProperties listener;

			plf::colony<VoiceImpl> voices;
			std::vector<float> buffer;

			void removeVoice(void *p)
			{
				voices.erase(voices.get_iterator_from_pointer((VoiceImpl *)p));
			}

			Holder<VoiceProperties> createVoice()
			{
				VoiceImpl *p = &*voices.emplace();
				Holder<VoiceProperties> h(p, p, Delegate<void(void *)>().bind<VoicesMixerImpl, &VoicesMixerImpl::removeVoice>(this));
				// todo init properties
				return templates::move(h);
			}

			real voiceGain(const VoiceImpl &v) const
			{
				real gain = listener.gain * v.gain;
				if (v.position.valid())
				{
					const real dist = max(distance(v.position, listener.position), v.referenceDistance);
					const real att = v.referenceDistance / (v.referenceDistance + v.rolloffFactor * listener.rolloffFactor * (dist - v.referenceDistance));
					gain *= att;
				}
				return gain;
			}

			void process(const SoundCallbackData &data)
			{
				CAGE_ASSERT(data.buffer.size() == data.frames * data.channels);

				for (float &dst : data.buffer)
					dst = 0;

				buffer.resize(data.buffer.size());
				for (const VoiceImpl &v : voices)
				{
					const real gain = voiceGain(v);
					if (gain < 1e-6)
						return;
					SoundCallbackData d = data;
					d.buffer = buffer;
					v.callback(d);
					auto src = buffer.begin();
					for (float &dst : data.buffer)
						dst += *src++ * gain.value;
				}
			}
		};
	}

	Holder<VoiceProperties> VoicesMixer::newVoice()
	{
		VoicesMixerImpl *impl = (VoicesMixerImpl *)this;
		return impl->createVoice();
	}

	ListenerProperties &VoicesMixer::listener()
	{
		VoicesMixerImpl *impl = (VoicesMixerImpl *)this;
		return impl->listener;
	}

	void VoicesMixer::process(const SoundCallbackData &data)
	{
		VoicesMixerImpl *impl = (VoicesMixerImpl *)this;
		impl->process(data);
	}

	Holder<VoicesMixer> newVoicesMixer(const VoicesMixerCreateConfig &config)
	{
		return detail::systemArena().createImpl<VoicesMixer, VoicesMixerImpl>(config);
	}
}
