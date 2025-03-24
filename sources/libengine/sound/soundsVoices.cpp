#include <algorithm>
#include <vector>

#include <cage-core/audioChannelsConverter.h>
#include <cage-core/audioDirectionalConverter.h>
#include <cage-core/sampleRateConverter.h>
#include <cage-engine/sound.h>
#include <cage-engine/soundsVoices.h>

namespace cage
{
	namespace
	{
		class VoicesMixerImpl;

		class VoiceImpl : public Voice
		{
		public:
			VoicesMixerImpl *mixer = nullptr;

			VoiceImpl(VoicesMixerImpl *mixer);
			~VoiceImpl();
		};

		class VoicesMixerImpl : public VoicesMixer
		{
		public:
			std::vector<VoiceImpl *> voices;
			Holder<SampleRateConverter> rateConv1, rateConvT;
			Holder<AudioDirectionalConverter> dirConv;
			Holder<AudioChannelsConverter> chansConv;
			std::vector<float> tmp1, tmp2;

			~VoicesMixerImpl()
			{
				for (auto it : voices)
					it->mixer = nullptr;
				voices.clear();
			}

			Holder<Voice> createVoice() { return systemMemory().createImpl<Voice, VoiceImpl>(this); }

			Real voiceGain(const VoiceImpl &v) const
			{
				Real att = 1;
				if (v.position.valid())
				{
					CAGE_ASSERT(v.maxDistance > v.minDistance);
					CAGE_ASSERT(v.minDistance > 0);
					const Real dist = clamp(distance(v.position, this->position), v.minDistance, v.maxDistance);
					switch (v.attenuation)
					{
						case SoundAttenuationEnum::None:
							break;
						case SoundAttenuationEnum::Linear:
							att = 1 - (dist - v.minDistance) / (v.maxDistance - v.minDistance);
							break;
						case SoundAttenuationEnum::Logarithmic:
							att = 1 - log(dist - v.minDistance + 1) / log(v.maxDistance - v.minDistance + 1);
							break;
						case SoundAttenuationEnum::InverseSquare:
							att = saturate(2 / (1 + sqr(dist / (v.minDistance + 1))));
							break;
						default:
							CAGE_THROW_ERROR(Exception, "invalid sound attenuation enum");
					}
				}
				CAGE_ASSERT(valid(att) && att >= 0 && att <= 1 + 1e-7);
				const Real gain = this->gain * v.gain * att;
				CAGE_ASSERT(valid(gain) && gain >= 0 && gain.finite());
				return gain;
			}

			void processVoice(VoiceImpl &v, const SoundCallbackData &data)
			{
				CAGE_ASSERT(!v.callback != !v.sound);

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
				else if (v.sound)
				{
					const uint32 channels = v.sound->channels();
					const uint32 sampleRate = v.sound->sampleRate();
					const sintPtr startFrame = numeric_cast<sintPtr>((data.time - v.startTime) * sampleRate / 1'000'000);
					const uintPtr frames = numeric_cast<uintPtr>(uint64(data.frames) * sampleRate / data.sampleRate);
					tmp1.resize(frames * channels);
					v.sound->decode(startFrame, tmp1, v.loop);

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
				else
					return;

				// apply spatial conversion
				if (spatial && data.channels > 1)
				{
					CAGE_ASSERT(tmp1.size() == data.frames);
					tmp2.resize(data.frames * data.channels);
					AudioDirectionalProcessConfig cfg;
					cfg.listenerOrientation = this->orientation;
					cfg.listenerPosition = this->position;
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
					chansConv = newAudioChannelsConverter();

				detail::memset(data.buffer.data(), 0, data.buffer.size() * sizeof(float));
				for (VoiceImpl *v : voices)
					processVoice(*v, data);
			}
		};

		VoiceImpl::VoiceImpl(VoicesMixerImpl *mixer) : mixer(mixer)
		{
			CAGE_ASSERT(mixer);
			mixer->voices.push_back(this);
		}

		VoiceImpl::~VoiceImpl()
		{
			if (mixer)
			{
				std::erase(mixer->voices, this);
				mixer = nullptr;
			}
		}
	}

	Holder<Voice> VoicesMixer::newVoice()
	{
		VoicesMixerImpl *impl = (VoicesMixerImpl *)this;
		return impl->createVoice();
	}

	void VoicesMixer::process(const SoundCallbackData &data)
	{
		VoicesMixerImpl *impl = (VoicesMixerImpl *)this;
		impl->process(data);
	}

	Holder<VoicesMixer> newVoicesMixer()
	{
		return systemMemory().createImpl<VoicesMixer, VoicesMixerImpl>();
	}
}
