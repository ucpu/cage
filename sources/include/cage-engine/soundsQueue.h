#ifndef guard_soundsQueue_h_fcg45hj5dftghj
#define guard_soundsQueue_h_fcg45hj5dftghj

#include <cage-engine/soundCommon.h>

namespace cage
{
	class Sound;
	class AssetsManager;

	struct CAGE_ENGINE_API SoundEventConfig
	{
		Time maxDelay = Time(2'000'000);
		sint32 priority = 0;
		Real gain = 1;
	};

	class CAGE_ENGINE_API SoundsQueue : private Immovable
	{
	public:
		uint32 maxSounds = 30;
		Real maxGain = Real::Infinity(); // all sounds will have proportionally reduced gain when number of sounds reaches this threshold
		Real gain = 1; // linear amplitude multiplier

		void play(Holder<Sound> sound, const SoundEventConfig &cfg = {});
		void play(AssetId soundId, const SoundEventConfig &cfg = {});
		void preload(AssetId soundId);
		void stop();

		bool playing() const;
		Time elapsedTime() const;
		Time remainingTime() const;

		// called in sound thread
		void process(const SoundCallbackData &data);

		// must be synchronized
		void purge();
	};

	CAGE_ENGINE_API Holder<SoundsQueue> newSoundsQueue(AssetsManager *assets);
}

#endif // guard_soundsQueue_h_fcg45hj5dftghj
