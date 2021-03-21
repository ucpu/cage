#ifndef guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
#define guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC

#include "soundCommon.h"

namespace cage
{
	class CAGE_ENGINE_API Sound : private Immovable
	{
	public:
		real referenceDistance = 1; // minimum distance to apply attenuation
		real rolloffFactor = 1; // distance multiplier
		real gain = 1; // linear amplitude multiplier

		bool loopBeforeStart = false;
		bool loopAfterEnd = false;

		void initialize(Holder<Audio> &&audio);

		uint64 frames() const;
		uint32 channels() const;
		uint32 sampleRate() const;
		uint64 duration() const; // microseconds

		// looping is handled here but attenuation and gain are not
		void decode(uint64 startFrame, PointerRange<float> buffer);

		// requires matching sample rate and channels
		// looping is handled here but attenuation and gain are not
		void process(const SoundCallbackData &data);
	};

	CAGE_ENGINE_API Holder<Sound> newSound();

	CAGE_ENGINE_API AssetScheme genAssetSchemeSound(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexSound = 20;
}

#endif // guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
