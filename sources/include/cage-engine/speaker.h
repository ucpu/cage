#ifndef guard_speaker_h_sdfh4df6h4drt6see
#define guard_speaker_h_sdfh4df6h4drt6see

#include <cage-engine/soundCommon.h>

namespace cage
{
	class CAGE_ENGINE_API Speaker : private Immovable
	{
	public:
		uint32 channels() const;
		uint32 sampleRate() const; // frames per second
		uint32 latency() const; // number of frames

		void start();
		void stop();
		bool running() const;

		void process(uint64 timeStamp, uint64 expectedPeriod);
	};

	struct CAGE_ENGINE_API SpeakerCreateConfig
	{
		String name;
		String deviceId;
		Delegate<void(const SoundCallbackData &)> callback;
		uint32 channels = 0;
		uint32 sampleRate = 0;

		// true -> use method process to request a call of the callback (in the same thread) to fill in the ring buffer
		// false -> the callback is called automatically (in the high-priority audio dedicated thread), the process method is ignored
		bool ringBuffer = true;
	};

	CAGE_ENGINE_API Holder<Speaker> newSpeaker(const SpeakerCreateConfig &config);
}

#endif // guard_speaker_h_sdfh4df6h4drt6see
