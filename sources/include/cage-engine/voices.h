#ifndef guard_voices_h_ies6yxd44kld6
#define guard_voices_h_ies6yxd44kld6

#include <cage-engine/soundCommon.h>

namespace cage
{
	class Sound;

	struct CAGE_ENGINE_API Voice : private Noncopyable
	{
		Holder<Sound> sound; // use exactly one of sound or callback
		Delegate<void(const SoundCallbackData &)> callback;
		Vec3 position = Vec3::Nan();
		sint64 startTime = 0;
		SoundAttenuationEnum attenuation = SoundAttenuationEnum::Logarithmic;
		Real minDistance = 1;
		Real maxDistance = 500;
		Real gain = 1; // linear amplitude multiplier
		bool loop = false;
	};

	class CAGE_ENGINE_API Listener : private Noncopyable
	{
	public:
		Quat orientation;
		Vec3 position;
		uint32 maxActiveVoices = 100;
		Real gain = 1; // linear amplitude multiplier
	};

	class CAGE_ENGINE_API VoicesMixer : private Immovable
	{
	public:
		Holder<Voice> newVoice();

		Listener &listener();

		void process(const SoundCallbackData &data);
	};

	CAGE_ENGINE_API Holder<VoicesMixer> newVoicesMixer();
}

#endif // guard_voices_h_ies6yxd44kld6
