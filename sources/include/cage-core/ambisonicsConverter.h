#ifndef guard_ambisonicsConverter_h_dt45rzr6t54uh185
#define guard_ambisonicsConverter_h_dt45rzr6t54uh185

#include "math.h"

namespace cage
{
	struct CAGE_CORE_API AmbisonicsData
	{
		vec3 direction;
	};

	class CAGE_CORE_API AmbisonicsConverter : private Immovable
	{
	public:
		void process(PointerRange<const float> srcMono, PointerRange<float> dstPoly, const AmbisonicsData &data);
	};

	struct CAGE_CORE_API AmbisonicsConverterCreateConfig
	{
		uint32 channels = 0;

		AmbisonicsConverterCreateConfig(uint32 channels) : channels(channels)
		{}
	};

	CAGE_CORE_API Holder<AmbisonicsConverter> newAmbisonicsConverter(const AmbisonicsConverterCreateConfig &config);
}

#endif // guard_ambisonicsConverter_h_dt45rzr6t54uh185
