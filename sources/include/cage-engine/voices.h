#ifndef guard_voices_h_ies6yxd44kld6
#define guard_voices_h_ies6yxd44kld6

#include <cage-engine/soundCommon.h>

namespace cage
{
	class Sound;

	struct CAGE_ENGINE_API Voice
	{
		Holder<Sound> sound; // use exactly one of sound or callback
		Delegate<void(const SoundCallbackData &)> callback;
		Vec3 position = Vec3::Nan();
		sint64 startTime = 0;
		Real gain = 1; // linear amplitude multiplier
	};

	struct CAGE_ENGINE_API Listener
	{
		Quat orientation;
		Vec3 position;
		Real rolloffFactor = 1; // distance multiplier
		Real gain = 1; // linear amplitude multiplier
		uint32 maxActiveVoices = 100;
	};

	class CAGE_ENGINE_API VoicesMixer : private Immovable
	{
	public:
		Holder<Voice> newVoice();

		Listener &listener();

		void process(const SoundCallbackData &data);
	};

	struct CAGE_ENGINE_API VoicesMixerCreateConfig
	{};

	CAGE_ENGINE_API Holder<VoicesMixer> newVoicesMixer(const VoicesMixerCreateConfig &config);
}

#endif // guard_voices_h_ies6yxd44kld6
