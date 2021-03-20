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
		//vec3 velocity;
		//vec3 direction;
		sint64 startTime = 0;
		//sint32 priority = 0; // higher priority is less likely to be deactivated
		//vec3 attenuation = vec3(0, 0, 1); // constant, linear, quadratic
		//real speedOfSound = 343.3; // meters per second
		real intensity = 1;
	};

	struct CAGE_ENGINE_API ListenerProperties
	{
		quat orientation;
		vec3 position;
		//vec3 velocity;
		real intensity = 1;
		uint32 maxActiveVoices = 100;
		//bool dopplerEffect = true;
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
