#include <cage-core/core.h>
#include <cage-core/math.h>
#include "private.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/sound.h>

namespace cage
{
	namespace
	{
		class volumeFilterImpl : public volumeFilter
		{
		public:
			volumeFilterImpl(soundContext *context)
			{
				filter = newMixingFilter(context);
				filter->execute.bind<volumeFilterImpl, &volumeFilterImpl::exe>(this);
			}

			void exe(const mixingFilterApi &api)
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

	holder<volumeFilter> newVolumeFilter(soundContext *context)
	{
		return detail::systemArena().createImpl<volumeFilter, volumeFilterImpl>(context);
	}
}
