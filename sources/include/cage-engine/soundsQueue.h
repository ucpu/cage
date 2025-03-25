#ifndef guard_soundsQueue_h_fcg45hj5dftghj
#define guard_soundsQueue_h_fcg45hj5dftghj

#include <cage-engine/soundCommon.h>

namespace cage
{
	class Sound;
	class AssetsManager;

	struct CAGE_ENGINE_API SoundEventConfig
	{
		uint64 maxDelay = 2'000'000;
		sint32 priority = 0;
		Real gain = 1;
	};

	class CAGE_ENGINE_API SoundsQueue : private Immovable
	{
	public:
		uint32 maxActiveSounds = 100;
		Real gain = 1; // linear amplitude multiplier

		void play(Holder<Sound> sound, const SoundEventConfig &cfg = {});
		void play(uint32 soundId, const SoundEventConfig &cfg = {});

		bool playing() const;
		uint64 elapsedTime() const;
		uint64 remainingTime() const;

		// called in sound thread
		void process(const SoundCallbackData &data);

		// must be synchronized
		void purge();
	};

	CAGE_ENGINE_API Holder<SoundsQueue> newSoundsQueue(AssetsManager *assets);
}

#endif // guard_soundsQueue_h_fcg45hj5dftghj
