#ifndef guard_sampleRateConverter_h_ds54hgz56dse3t544h
#define guard_sampleRateConverter_h_ds54hgz56dse3t544h

#include "core.h"

namespace cage
{
	class CAGE_CORE_API SampleRateConverter : private Immovable
	{
	public:
		uint32 channels() const;

		void convert(PointerRange<const float> src, PointerRange<float> dst, double ratio);
		void convert(PointerRange<const float> src, PointerRange<float> dst, double startRatio, double endRatio);
	};

	struct SampleRateConverterCreateConfig
	{
#ifdef CAGE_DEBUG
		static constexpr uint32 DefaultQuality = 1;
#else
		static constexpr uint32 DefaultQuality = 3;
#endif // CAGE_DEBUG

		uint32 channels = 0;
		uint32 quality = DefaultQuality; // 0 = nearest neighbor, 1 = linear, 2 = low, 3 = medium, 4 = high

		SampleRateConverterCreateConfig(uint32 channels) : channels(channels)
		{}
	};

	CAGE_CORE_API Holder<SampleRateConverter> newSampleRateConverter(const SampleRateConverterCreateConfig &config);
}

#endif // guard_sampleRateConverter_h_ds54hgz56dse3t544h
