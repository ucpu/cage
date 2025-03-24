#ifndef guard_soundsQueue_h_fcg45hj5dftghj
#define guard_soundsQueue_h_fcg45hj5dftghj

#include <cage-engine/soundCommon.h>

namespace cage
{
	class Sound;
	class AssetsManager;

	class CAGE_ENGINE_API SoundsQueue : private Immovable
	{
	public:
		uint32 maxActiveSounds = 100;
		Real gain = 1; // linear amplitude multiplier

		void play(Holder<Sound> sound, Real gain = 1);
		void play(uint32 soundId, Real gain = 1);
		void stop();
		bool playing() const;

		// called in sound thread
		void process(const SoundCallbackData &data);

		// must be synchronized
		void purge();
	};

	CAGE_ENGINE_API Holder<SoundsQueue> newSoundsQueue(AssetsManager *assets);
}

#endif // guard_soundsQueue_h_fcg45hj5dftghj
