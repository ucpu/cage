#include <cage-core/audioDirectionalConverter.h>
#include <cage-core/sampleRateConverter.h>
#include <cage-core/audioChannelsConverter.h>

#include <cage-engine/voices.h>
#include <cage-engine/sound.h>

#include <plf_colony.h>

#include <vector>
#include <utility>

namespace cage
{
	namespace
	{
		struct VoiceImpl : public Voice
		{};

		class VoicesMixerImpl : public VoicesMixer
		{
		public:
			VoicesMixerImpl(const VoicesMixerCreateConfig &config)
			{}

			Listener listener;
			plf::colony<VoiceImpl> voices;
			Holder<SampleRateConverter> rateConv;
			Holder<AudioDirectionalConverter> dirConv;
			Holder<AudioChannelsConverter> chansConv;
			std::vector<float> tmp1, tmp2;

			void removeVoice(void *p)
			{
				voices.erase(voices.get_iterator_from_pointer((VoiceImpl *)p));
			}

			Holder<Voice> createVoice()
			{
				VoiceImpl *p = &*voices.emplace();
				Holder<Voice> h(p, p, Delegate<void(void *)>().bind<VoicesMixerImpl, &VoicesMixerImpl::removeVoice>(this));
				// todo init properties
				return templates::move(h);
			}

			real attenuation(const vec3 &position, real referenceDistance, real rolloffFactor) const
			{
				const real dist = max(distance(position, listener.position), referenceDistance);
				return referenceDistance / (referenceDistance + rolloffFactor * listener.rolloffFactor * (dist - referenceDistance));
			}

			real voiceGain(const VoiceImpl &v) const
			{
				real gain = listener.gain * v.gain;
				if (v.position.valid())
				{
					if (v.sound)
						gain *= attenuation(v.position, v.sound->referenceDistance, v.sound->rolloffFactor);
					else
						gain *= attenuation(v.position, 1, 1);
				}
				return gain;
			}

			void processVoice(VoiceImpl &v, const SoundCallbackData &data)
			{
				const real gain = voiceGain(v);
				if (gain < 1e-6)
					return;
				if (v.callback)
				{
					SoundCallbackData d = data;
					if (v.position.valid())
						d.channels = 1;
					tmp1.resize(d.frames * d.channels);
					d.buffer = tmp1;
					v.callback(d);
				}
				else
				{
					const uint32 channels = v.sound->channels();
					const uint32 sampleRate = v.sound->sampleRate();
					const uint64 startFrame = data.time * sampleRate / 1000000;
					const uint64 frames = data.frames * sampleRate / data.sampleRate;
					tmp1.resize(frames * channels);
					v.sound->decode(startFrame, tmp1);
					if (sampleRate == data.sampleRate)
						std::swap(tmp1, tmp2);
					else
					{
						tmp2.resize(data.frames * channels);
						rateConv->convert(tmp1, tmp2, data.sampleRate / (double)sampleRate);
					}
					if (v.position.valid())
					{
						if (channels == 1)
							std::swap(tmp1, tmp2);
						else
						{
							tmp1.resize(data.frames);
							chansConv->convert(tmp2, tmp1, channels, 1);
						}
					}
					else
					{
						if (channels == data.channels)
							std::swap(tmp1, tmp2);
						else
						{
							tmp1.resize(data.frames * data.channels);
							chansConv->convert(tmp2, tmp1, channels, data.channels);
						}
					}
				}
				if (v.position.valid())
				{
					CAGE_ASSERT(tmp1.size() == data.frames);
					tmp2.resize(data.frames * data.channels);
					AudioDirectionalData cfg;
					cfg.listenerOrientation = listener.orientation;
					cfg.listenerPosition = listener.position;
					cfg.sourcePosition = v.position;
					dirConv->process(tmp1, tmp2, cfg);
				}
				else
					std::swap(tmp1, tmp2);
				CAGE_ASSERT(tmp2.size() == data.buffer.size());
				auto src = tmp2.begin();
				for (float &dst : data.buffer)
					dst += *src++ * gain.value;
			}

			void process(const SoundCallbackData &data)
			{
				CAGE_ASSERT(data.buffer.size() == data.frames * data.channels);

				if (!rateConv || rateConv->channels() != data.channels)
					rateConv = newSampleRateConverter(data.channels);
				if (!dirConv || dirConv->channels() != data.channels)
					dirConv = newAudioDirectionalConverter(data.channels);
				if (!chansConv)
					chansConv = newAudioChannelsConverter({});

				for (float &dst : data.buffer)
					dst = 0;
				for (VoiceImpl &v : voices)
				{
					CAGE_ASSERT(!v.callback != !v.sound);
					processVoice(v, data);
				}
			}
		};
	}

	Holder<Voice> VoicesMixer::newVoice()
	{
		VoicesMixerImpl *impl = (VoicesMixerImpl *)this;
		return impl->createVoice();
	}

	Listener &VoicesMixer::listener()
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
