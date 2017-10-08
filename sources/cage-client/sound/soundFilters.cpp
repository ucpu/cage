#include <cage-core/core.h>
#include <cage-core/math.h>
#include "private.h"

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/sound.h>

namespace cage
{
	namespace
	{
		class volumeFilterImpl : public volumeFilterClass
		{
		public:
			volumeFilterImpl(soundContextClass *context)
			{
				filter = newFilter(context);
				filter->execute.bind<volumeFilterImpl, &volumeFilterImpl::exe>(this);
			}

			void exe(const filterApiStruct &api)
			{
				CAGE_ASSERT_RUNTIME(volume >= 0, volume.value);
				if (volume < 1e-7)
					return;
				api.input(api.output);
				for (uint32 i = 0, e = api.output.frames * api.output.channels; i < e; i++)
					api.output.buffer[i] *= volume.value;
			}
		};
	}

	holder<volumeFilterClass> newFilterVolume(soundContextClass *context)
	{
		return detail::systemArena().createImpl <volumeFilterClass, volumeFilterImpl>(context);
	}
}