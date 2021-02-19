#ifndef guard_polytone_h_C930FD49904A491DBB9CF3D0AE972EB2
#define guard_polytone_h_C930FD49904A491DBB9CF3D0AE972EB2

#include "core.h"

namespace cage
{
	enum class PolytoneFormatEnum : uint32
	{
		S16 = 1, // 16 bit normalized (native endianness)
		S32 = 2, // 32 bit normalized (native endianness)
		Float = 3,
		Vorbis = 4,

		Default = m, // used only for decoding, it will use original format from the file
	};

	class CAGE_CORE_API Polytone : private Immovable
	{
	public:
		void clear();
		void initialize(uint64 frames, uint32 channels, uint32 sampleRate = 48000, PolytoneFormatEnum format = PolytoneFormatEnum::Float);
		Holder<Polytone> copy() const;

		// import from raw memory
		void importRaw(PointerRange<const char> buffer, uint64 frames, uint32 channels, uint32 sampleRate, PolytoneFormatEnum format);

		// sound decode
		void importBuffer(PointerRange<const char> buffer, PolytoneFormatEnum requestedFormat = PolytoneFormatEnum::Default);
		void importFile(const string &filename, PolytoneFormatEnum requestedFormat = PolytoneFormatEnum::Default);

		// sound encode
		Holder<PointerRange<char>> exportBuffer(const string &format = ".wav") const;
		void exportFile(const string &filename) const;

		// the format must match
		// the polytone must outlive the view
		PointerRange<const sint16> rawViewS16() const;
		PointerRange<const sint32> rawViewS32() const;
		PointerRange<const float> rawViewFloat() const;
		PointerRange<const char> rawViewVorbis() const;

		uint64 frames() const;
		uint32 channels() const;
		uint32 sampleRate() const;
		PolytoneFormatEnum format() const;
		uint64 duration() const; // microsecond

		float value(uint64 f, uint32 c) const;
		void value(uint64 f, uint32 c, float v);
		void value(uint64 f, uint32 c, const real &v);
	};

	CAGE_CORE_API Holder<Polytone> newPolytone();

	CAGE_CORE_API void polytoneSetSampleRate(Polytone *snd, uint32 sampleRate); // preserve number of frames and change duration
	CAGE_CORE_API void polytoneConvertSampleRate(Polytone *snd, uint32 sampleRate); // preserve duration and change number of frames
	CAGE_CORE_API void polytoneConvertFrames(Polytone *snd, uint64 frames); // preserve duration and change sample rate
	CAGE_CORE_API void polytoneConvertChannels(Polytone *snd, uint32 channels, PointerRange<float> matrix);
	CAGE_CORE_API void polytoneConvertFormat(Polytone *snd, PolytoneFormatEnum format);

	// copies parts of a polytone into another polytone
	// if the target and source polytones are the same instance, the source range and target range cannot overlap
	// if the target polytone is not initialized and the targetFrameOffset is zero, it will be initialized with channels and format of the source polytone
	// if the polytones have different format, transferred samples will be converted to the target format
	// both polytones must have same number of channels
	// sample rate is ignored (except when initializing new polytone)
	CAGE_CORE_API void polytoneBlit(const Polytone *source, Polytone *target, uint64 sourceFrameOffset, uint64 targetFrameOffset, uint64 frames);

	// stateful decoder
	class CAGE_CORE_API PolytoneStream : private Immovable
	{
	public:
		const Polytone *source() const;

		void decode(uint64 frame, PointerRange<float> buffer);
	};

	CAGE_CORE_API Holder<PolytoneStream> newPolytoneStream(Holder<Polytone> &&polytone);

	class CAGE_CORE_API SampleRateConverter : private Immovable
	{
	public:
		uint32 channels() const;

		void convert(PointerRange<const float> src, PointerRange<float> dst, double ratio);
		void convert(PointerRange<const float> src, PointerRange<float> dst, double startRatio, double endRatio);
	};

	struct SampleRateConverterCreateConfig
	{
		uint32 channels = 0;
		// quality

		SampleRateConverterCreateConfig(uint32 channels) : channels(channels)
		{}
	};

	CAGE_CORE_API Holder<SampleRateConverter> newSampleRateConverter(const SampleRateConverterCreateConfig &config);
}

#endif // guard_polytone_h_C930FD49904A491DBB9CF3D0AE972EB2
