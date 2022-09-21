#ifndef guard_audio_h_C930FD49904A491DBB9CF3D0AE972EB2
#define guard_audio_h_C930FD49904A491DBB9CF3D0AE972EB2

#include "core.h"

namespace cage
{
	enum class AudioFormatEnum : uint32
	{
		S16 = 1, // 16 bit normalized (native endianness)
		S32 = 2, // 32 bit normalized (native endianness)
		Float = 3,
		Vorbis = 4,

		Default = m, // used only for decoding, it will use original format from the file
	};

	class CAGE_CORE_API Audio : private Immovable
	{
	public:
		void clear();
		void initialize(uintPtr frames, uint32 channels, uint32 sampleRate = 48000, AudioFormatEnum format = AudioFormatEnum::Float);
		Holder<Audio> copy() const;

		// import from raw memory
		void importRaw(PointerRange<const char> buffer, uintPtr frames, uint32 channels, uint32 sampleRate, AudioFormatEnum format);

		// sound decode
		void importBuffer(PointerRange<const char> buffer, AudioFormatEnum requestedFormat = AudioFormatEnum::Default);
		void importFile(const String &filename, AudioFormatEnum requestedFormat = AudioFormatEnum::Default);

		// sound encode
		Holder<PointerRange<char>> exportBuffer(const String &format = ".wav") const;
		void exportFile(const String &filename) const;

		// the format must match
		// the audio must outlive the view
		PointerRange<const sint16> rawViewS16() const;
		PointerRange<const sint32> rawViewS32() const;
		PointerRange<const float> rawViewFloat() const;
		PointerRange<const char> rawViewVorbis() const;

		uintPtr frames() const;
		uint32 channels() const;
		uint32 sampleRate() const;
		AudioFormatEnum format() const;
		uint64 duration() const; // microseconds

		float value(uintPtr f, uint32 c) const;
		void value(uintPtr f, uint32 c, float v);
		void value(uintPtr f, uint32 c, const Real &v);

		void decode(uintPtr startFrame, PointerRange<float> buffer) const;
	};

	CAGE_CORE_API Holder<Audio> newAudio();
}

#endif // guard_audio_h_C930FD49904A491DBB9CF3D0AE972EB2
