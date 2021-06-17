#include <cage-engine/speakerList.h>

#include <vector>
#include <cubeb/cubeb.h>

namespace cage
{
	void cageCheckCubebError(int code);
	cubeb *cageCubebInitializeFunc();

	namespace
	{
		struct Devices : Immovable, public cubeb_device_collection
		{
			Devices()
			{
				cageCheckCubebError(cubeb_enumerate_devices(context, CUBEB_DEVICE_TYPE_OUTPUT, this));
			}

			~Devices()
			{
				cubeb_device_collection_destroy(context, this);
			}

		private:
			cubeb *context = cageCubebInitializeFunc();
		};

		class SpeakerListImpl : public SpeakerList
		{
		public:
			std::vector<SpeakerDevice> devices;
			uint32 defaultDevice = m;

			SpeakerListImpl()
			{
				Devices collection;
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
					s.available = d.state == CUBEB_DEVICE_STATE_ENABLED;
					devices.push_back(s);
				}
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
		return systemMemory().createImpl<SpeakerList, SpeakerListImpl>();
	}
}
