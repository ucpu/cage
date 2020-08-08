#include <cage-core/pointerRangeHolder.h>
#include <cage-engine/sound.h>
#include <cage-engine/speakerList.h>
#include "sound/private.h"

#include <vector>
#include <cubeb/cubeb.h>

namespace cage
{
	namespace
	{
		class SpeakerListImpl : public SpeakerList
		{
		public:
			std::vector<SpeakerDevice> devices;
			uint32 defaultDevice = m;

			SpeakerListImpl()
			{
				Holder<SoundContext> cnx = newSoundContext(SoundContextCreateConfig());
				cubeb *snd = soundioFromContext(cnx.get());
				cubeb_device_collection collection = {};
				checkSoundIoError(cubeb_enumerate_devices(snd, CUBEB_DEVICE_TYPE_OUTPUT , &collection));

				devices.reserve(collection.count);
				for (uint32 index = 0; index < collection.count; index++)
				{
					const cubeb_device_info &d = collection.device[index];
					if (d.preferred)
						defaultDevice = numeric_cast<uint32>(devices.size());
					SpeakerDevice s;
					s.id = d.device_id ? d.device_id : "";
					s.name = d.friendly_name ? d.friendly_name : "";
					s.group = d.group_id ? d.group_id : "";
					s.vendor = d.vendor_name ? d.vendor_name : "";
					s.channels = d.max_channels;
					s.minSamplerate = d.min_rate;
					s.maxSamplerate = d.max_rate;
					s.defaultSamplerate = d.default_rate;
					devices.push_back(s);
				}

				cubeb_device_collection_destroy(snd, &collection);
			}
		};
	}

	uint32 SpeakerList::defaultDevice() const
	{
		const SpeakerListImpl *impl = (const SpeakerListImpl *)this;
		return impl->defaultDevice;
	}

	PointerRange<const SpeakerDevice> SpeakerList::devices() const
	{
		const SpeakerListImpl *impl = (const SpeakerListImpl *)this;
		return impl->devices;
	}

	Holder<SpeakerList> newSpeakerList()
	{
		return detail::systemArena().createImpl<SpeakerList, SpeakerListImpl>();
	}
}
