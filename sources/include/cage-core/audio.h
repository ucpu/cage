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
		void initialize(uint64 frames, uint32 channels, uint32 sampleRate = 48000, AudioFormatEnum format = AudioFormatEnum::Float);
		Holder<Audio> copy() const;

		// import from raw memory
		void importRaw(PointerRange<const char> buffer, uint64 frames, uint32 channels, uint32 sampleRate, AudioFormatEnum format);

		// sound decode
		void importBuffer(PointerRange<const char> buffer, AudioFormatEnum requestedFormat = AudioFormatEnum::Default);
		void importFile(const string &filename, AudioFormatEnum requestedFormat = AudioFormatEnum::Default);

		// sound encode
		Holder<PointerRange<char>> exportBuffer(const string &format = ".wav") const;
		void exportFile(const string &filename) const;

		// the format must match
		// the audio must outlive the view
		PointerRange<const sint16> rawViewS16() const;
		PointerRange<const sint32> rawViewS32() const;
		PointerRange<const float> rawViewFloat() const;
		PointerRange<const char> rawViewVorbis() const;

		uint64 frames() const;
		uint32 channels() const;
		uint32 sampleRate() const;
		AudioFormatEnum format() const;
		uint64 duration() const; // microseconds

		float value(uint64 f, uint32 c) const;
		void value(uint64 f, uint32 c, float v);
		void value(uint64 f, uint32 c, const real &v);
	};

	CAGE_CORE_API Holder<Audio> newAudio();

	CAGE_CORE_API void audioSetSampleRate(Audio *snd, uint32 sampleRate); // preserve number of frames and change duration
	CAGE_CORE_API void audioConvertSampleRate(Audio *snd, uint32 sampleRate, uint32 quality = 4); // preserve duration and change number of frames
	CAGE_CORE_API void audioConvertFrames(Audio *snd, uint64 frames, uint32 quality = 4); // preserve duration and change sample rate
	CAGE_CORE_API void audioConvertFormat(Audio *snd, AudioFormatEnum format);

	// copies parts of an audio into another audio
	// if the target and source audios are the same instance, the source range and target range cannot overlap
	// if the target audio is not initialized and the targetFrameOffset is zero, it will be initialized with channels and format of the source audio
	// if the audios have different format, transferred samples will be converted to the target format
	// both audios must have same number of channels
	// sample rate is ignored (except when initializing new audio)
	CAGE_CORE_API void audioBlit(const Audio *source, Audio *target, uint64 sourceFrameOffset, uint64 targetFrameOffset, uint64 frames);

	// stateful decoder
	class CAGE_CORE_API AudioStream : private Immovable
	{
	public:
		const Audio *source() const;

		void decode(uint64 frame, PointerRange<float> buffer);
	};

	CAGE_CORE_API Holder<AudioStream> newAudioStream(Holder<Audio> &&audio);
}

#endif // guard_audio_h_C930FD49904A491DBB9CF3D0AE972EB2
