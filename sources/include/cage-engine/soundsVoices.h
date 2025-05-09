#ifndef guard_soundsVoices_h_ies6yxd44kld6
#define guard_soundsVoices_h_ies6yxd44kld6

#include <cage-engine/soundCommon.h>

namespace cage
{
	class Sound;

	class CAGE_ENGINE_API Voice : private Immovable
	{
	public:
		Holder<Sound> sound; // use exactly one of sound or callback
		Delegate<void(const SoundCallbackData &)> callback;
		Vec3 position = Vec3::Nan();
		sint64 startTime = 0;
		SoundAttenuationEnum attenuation = SoundAttenuationEnum::Logarithmic;
		Real minDistance = 1;
		Real maxDistance = 500;
		Real gain = 1; // linear amplitude multiplier
		sint32 priority = 0;
		bool loop = false;
	};

	class CAGE_ENGINE_API VoicesMixer : private Immovable
	{
	public:
		Quat orientation;
		Vec3 position;
		uint32 maxActiveSounds = 100;
		Real maxGainThreshold = Real::Infinity(); // all sounds will have proportionally reduced gain when the sum of effective gains reaches this threshold
		Real gain = 1; // linear amplitude multiplier

		Holder<Voice> newVoice();

		void process(const SoundCallbackData &data);
	};

	CAGE_ENGINE_API Holder<VoicesMixer> newVoicesMixer();
}

#endif // guard_soundsVoices_h_ies6yxd44kld6
