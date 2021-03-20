#ifndef guard_voices_h_ies6yxd44kld6
#define guard_voices_h_ies6yxd44kld6

#include "soundCommon.h"

namespace cage
{
	struct CAGE_ENGINE_API VoiceProperties
	{
		Holder<Sound> sound; // optional holder
		Delegate<void(const SoundCallbackData &)> callback;
		vec3 position = vec3::Nan();
		sint64 startTime = 0;
		real referenceDistance = 1; // minimum distance to apply attenuation
		real rolloffFactor = 1; // distance multiplier
		real gain = 1; // linear amplitude multiplier
	};

	struct CAGE_ENGINE_API ListenerProperties
	{
		quat orientation;
		vec3 position;
		real rolloffFactor = 1; // distance multiplier
		real gain = 1; // linear amplitude multiplier
		uint32 maxActiveVoices = 100;
	};

	class CAGE_ENGINE_API VoicesMixer : Immovable
	{
	public:
		Holder<VoiceProperties> newVoice();

		ListenerProperties &listener();

		void process(const SoundCallbackData &data);
	};

	struct CAGE_ENGINE_API VoicesMixerCreateConfig
	{};

	CAGE_ENGINE_API Holder<VoicesMixer> newVoicesMixer(const VoicesMixerCreateConfig &config);
}

#endif // guard_voices_h_ies6yxd44kld6
