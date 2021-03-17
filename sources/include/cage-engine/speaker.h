#ifndef guard_speaker_h_sdfh4df6h4drt6see
#define guard_speaker_h_sdfh4df6h4drt6see

#include "core.h"

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

		void update(uint64 time);
	};

	struct CAGE_ENGINE_API SpeakerCreateConfig
	{
		string name;
		string deviceId;
		uint32 channels = 0;
		uint32 sampleRate = 0;

		// true -> use update to request a call of the callback (in the same thread) to fill in the ring buffer
		// false -> the callback is called automatically (in the high-priority audio dedicated thread), the update method is ignored
		bool ringBuffer = true;
	};

	struct CAGE_ENGINE_API SpeakerCallbackData
	{
		PointerRange<float> buffer;
		uint64 time = 0;
		uint32 channels = 0;
		uint32 frames = 0;
		uint32 sampleRate = 0;
	};

	CAGE_ENGINE_API Holder<Speaker> newSpeaker(const SpeakerCreateConfig &config, Delegate<void(const SpeakerCallbackData &)> callback);
}

#endif // guard_speaker_h_sdfh4df6h4drt6see
