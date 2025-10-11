#ifndef guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
#define guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC

#include <cage-engine/soundCommon.h>

namespace cage
{
	class Audio;

	class CAGE_ENGINE_API Sound : private Immovable
	{
		detail::StringBase<128> label;

	public:
		void setLabel(const String &name);

		void importAudio(Holder<Audio> &&audio);

		uintPtr frames() const;
		uint32 channels() const;
		uint32 sampleRate() const;
		uint64 duration() const; // microseconds

		void decode(sintPtr startFrame, PointerRange<float> buffer, bool loop) const;

		// requires matching sample rate and channels
		void process(const SoundCallbackData &data, bool loop) const;
	};

	CAGE_ENGINE_API Holder<Sound> newSound();
}

#endif // guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
