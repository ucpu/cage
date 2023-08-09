#include <vector>

#include <plf_colony.h>

#include <cage-core/audioChannelsConverter.h>
#include <cage-core/audioDirectionalConverter.h>
#include <cage-core/sampleRateConverter.h>
#include <cage-engine/sound.h>
#include <cage-engine/voices.h>

namespace cage
{
	namespace
	{
		struct VoiceImpl : public Voice
		{};

		class VoicesMixerImpl : public VoicesMixer
		{
		public:
			VoicesMixerImpl(const VoicesMixerCreateConfig &config) {}

			Listener listener;
			plf::colony<VoiceImpl> voices;
			Holder<SampleRateConverter> rateConv1, rateConvT;
			Holder<AudioDirectionalConverter> dirConv;
			Holder<AudioChannelsConverter> chansConv;
			std::vector<float> tmp1, tmp2;

			void removeVoice(void *p) { voices.erase(voices.get_iterator((VoiceImpl *)p)); }

			Holder<Voice> createVoice()
			{
				struct VoiceReference
				{
					VoiceImpl *v = nullptr;
					VoicesMixerImpl *m = nullptr;
					~VoiceReference() { m->voices.erase(m->voices.get_iterator(v)); }
				};
				Holder<VoiceReference> h = systemMemory().createHolder<VoiceReference>();
				h->v = &*voices.emplace();
				h->m = this;
				// todo init properties
				return Holder<Voice>(h->v, std::move(h));
			}

			Real attenuation(const Vec3 &position, Real referenceDistance, Real rolloffFactor) const
			{
				const Real dist = max(distance(position, listener.position), referenceDistance);
				return referenceDistance / (referenceDistance + rolloffFactor * listener.rolloffFactor * (dist - referenceDistance));
			}

			Real voiceGain(const VoiceImpl &v) const
			{
				Real gain = listener.gain * v.gain;
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
				const Real gain = voiceGain(v);
				if (gain < 1e-6)
					return;

				const bool spatial = v.position.valid();

				// decode source
				if (v.callback)
				{
					SoundCallbackData d = data;
					if (spatial)
						d.channels = 1;
					tmp1.resize(d.frames * d.channels);
					d.buffer = tmp1;
					v.callback(d);
				}
				else
				{
					const uint32 channels = v.sound->channels();
					const uint32 sampleRate = v.sound->sampleRate();
					const sintPtr startFrame = numeric_cast<sintPtr>((data.time - v.startTime) * sampleRate / 1000000);
					const uintPtr frames = numeric_cast<uintPtr>(uint64(data.frames) * sampleRate / data.sampleRate);
					tmp1.resize(frames * channels);
					v.sound->decode(startFrame, tmp1);

					// convert to 1 channel for spatial sound and to output channels otherwise
					if (spatial && channels != 1)
					{
						tmp2.resize(frames * 1);
						chansConv->convert(tmp1, tmp2, channels, 1);
						std::swap(tmp1, tmp2);
					}
					if (!spatial && channels != data.channels)
					{
						tmp2.resize(frames * data.channels);
						chansConv->convert(tmp1, tmp2, channels, data.channels);
						std::swap(tmp1, tmp2);
					}
					CAGE_ASSERT((tmp1.size() % (v.position.valid() ? 1 : data.channels)) == 0);

					// convert sample rate
					if (sampleRate != data.sampleRate)
					{
						if (spatial)
						{
							tmp2.resize(data.frames * 1);
							rateConv1->convert(tmp1, tmp2, data.sampleRate / (double)sampleRate);
						}
						else
						{
							tmp2.resize(data.frames * data.channels);
							rateConvT->convert(tmp1, tmp2, data.sampleRate / (double)sampleRate);
						}
						std::swap(tmp1, tmp2);
					}
				}

				// apply spatial conversion
				if (spatial && data.channels > 1)
				{
					CAGE_ASSERT(tmp1.size() == data.frames);
					tmp2.resize(data.frames * data.channels);
					AudioDirectionalProcessConfig cfg;
					cfg.listenerOrientation = listener.orientation;
					cfg.listenerPosition = listener.position;
					cfg.sourcePosition = v.position;
					dirConv->process(tmp1, tmp2, cfg);
					std::swap(tmp1, tmp2);
				}

				// add the result to accumulation buffer
				CAGE_ASSERT(tmp1.size() == data.buffer.size());
				auto src = tmp1.begin();
				for (float &dst : data.buffer)
					dst += *src++ * gain.value;
			}

			void process(const SoundCallbackData &data)
			{
				CAGE_ASSERT(data.buffer.size() == data.frames * data.channels);

				if (!rateConv1)
					rateConv1 = newSampleRateConverter(1);
				if (!rateConvT || rateConvT->channels() != data.channels)
					rateConvT = newSampleRateConverter(data.channels);
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
		return systemMemory().createImpl<VoicesMixer, VoicesMixerImpl>(config);
	}
}
