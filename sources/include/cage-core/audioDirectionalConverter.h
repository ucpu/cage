#ifndef guard_audioDirectionalConverter_h_dt45rzr6t54uh185
#define guard_audioDirectionalConverter_h_dt45rzr6t54uh185

#include "math.h"

namespace cage
{
	struct CAGE_CORE_API AudioDirectionalProcessConfig
	{
		quat listenerOrientation;
		vec3 listenerPosition;
		vec3 sourcePosition;
	};

	class CAGE_CORE_API AudioDirectionalConverter : private Immovable
	{
	public:
		uint32 channels() const;

		void process(PointerRange<const float> srcMono, PointerRange<float> dstPoly, const AudioDirectionalProcessConfig &config);
	};

	struct CAGE_CORE_API AudioDirectionalConverterCreateConfig
	{
		uint32 channels = 0;

		AudioDirectionalConverterCreateConfig(uint32 channels) : channels(channels)
		{}
	};

	CAGE_CORE_API Holder<AudioDirectionalConverter> newAudioDirectionalConverter(const AudioDirectionalConverterCreateConfig &config);
}

#endif // guard_audioDirectionalConverter_h_dt45rzr6t54uh185
