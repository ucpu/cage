#ifndef guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
#define guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC

#include "soundCommon.h"

namespace cage
{
	class CAGE_ENGINE_API Sound : private Immovable
	{
#ifdef CAGE_DEBUG
		detail::StringBase<64> debugName;
#endif // CAGE_DEBUG

	public:
		void setDebugName(const String &name);

		Real referenceDistance = 1; // minimum distance to apply attenuation
		Real rolloffFactor = 1; // distance multiplier
		Real gain = 1; // linear amplitude multiplier

		bool loopBeforeStart = false;
		bool loopAfterEnd = false;

		void initialize(Holder<Audio> &&audio);

		uintPtr frames() const;
		uint32 channels() const;
		uint32 sampleRate() const;
		uint64 duration() const; // microseconds

		// looping is handled here but attenuation and gain are not
		void decode(sintPtr startFrame, PointerRange<float> buffer);

		// requires matching sample rate and channels
		// looping is handled here but attenuation and gain are not
		void process(const SoundCallbackData &data);
	};

	CAGE_ENGINE_API Holder<Sound> newSound();

	CAGE_ENGINE_API AssetScheme genAssetSchemeSound();
	static constexpr uint32 AssetSchemeIndexSound = 20;
}

#endif // guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
