#include "private.h"

namespace cage
{
	namespace
	{
		class VolumeFilterImpl : public VolumeFilter
		{
		public:
			explicit VolumeFilterImpl(SoundContext *context)
			{
				filter = newMixingFilter(context);
				filter->execute.bind<VolumeFilterImpl, &VolumeFilterImpl::exe>(this);
			}

			void exe(const MixingFilterApi &api)
			{
				CAGE_ASSERT(volume >= 0, volume.value);
				if (volume < 1e-7)
					return;
				api.input(api.output);
				for (uint32 i = 0, e = api.output.frames * api.output.channels; i < e; i++)
					api.output.buffer[i] *= volume.value;
			}
		};
	}

	Holder<VolumeFilter> newVolumeFilter(SoundContext *context)
	{
		return detail::systemArena().createImpl<VolumeFilter, VolumeFilterImpl>(context);
	}
}
