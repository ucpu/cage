#include <vector>

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
		struct layoutInfo
		{
			string name;
			std::vector<SoundIoChannelId> channels;
		};

		void parseLayout(layoutInfo &info, SoundIoChannelLayout &layout)
		{
			info.name = layout.name;
			uint32 cc = layout.channel_count;
			info.channels.resize(cc);
			for (uint32 i = 0; i < cc; i++)
				info.channels[i] = layout.channels[i];
		}

		struct samplerateInfo
		{
			int min, max;
		};

		struct deviceInfo
		{
			string deviceId;
			string deviceName;
			bool raw;

			std::vector<layoutInfo> layouts;
			uint32 layoutCurrent;

			std::vector<samplerateInfo> samplerates;
			int samplerateCurrent;

			string formatCurrent;
			float latencyMin, latencyMax, latencyCurrent;

			deviceInfo() : raw(false), layoutCurrent(0), samplerateCurrent(0), latencyMin(0), latencyMax(0), latencyCurrent(0)
			{}
		};

		class speakerListImpl : public speakerListClass
		{
		public:
			SoundIo *soundio;
			std::vector<deviceInfo> infos;
			uint32 defaultDevice;

			speakerListImpl(bool inputs) : soundio(nullptr), defaultDevice(0)
			{
				holder<soundContextClass> cnx = newSoundContext(soundContextCreateConfig());
				soundio = soundioFromContext(cnx.get());
				if (inputs)
				{
					int count = soundio_input_device_count(soundio);
					infos.resize(count);
					defaultDevice = soundio_default_output_device_index(soundio);
					for (int i = 0; i < count; i++)
					{
						SoundIoDevice *device = soundio_get_input_device(soundio, i);
						parseDevice(infos[i], device);
						soundio_device_unref(device);
					}
				}
				else
				{
					int count = soundio_output_device_count(soundio);
					infos.resize(count);
					defaultDevice = soundio_default_input_device_index(soundio);
					for (int i = 0; i < count; i++)
					{
						SoundIoDevice *device = soundio_get_output_device(soundio, i);
						parseDevice(infos[i], device);
						soundio_device_unref(device);
					}
				}
			}

			void parseDevice(deviceInfo &d, SoundIoDevice *device)
			{
				// device
				d.deviceId = device->id;
				d.deviceName = device->name;
				d.raw = device->is_raw;
				d.formatCurrent = soundio_format_string(device->current_format);

				// layouts
				uint32 lc = device->layout_count;
				d.layoutCurrent = 0;
				d.layouts.resize(lc);
				for (uint32 i = 0; i < lc; i++)
				{
					parseLayout(d.layouts[i], device->layouts[i]);
					if (soundio_channel_layout_equal(&device->layouts[i], &device->current_layout))
						d.layoutCurrent = i;
				}

				// sample rates
				d.samplerateCurrent = device->sample_rate_current;
				uint32 sc = device->sample_rate_count;
				d.samplerates.resize(sc);
				for (uint32 i = 0; i < sc; i++)
				{
					d.samplerates[i].min = device->sample_rates[i].min;
					d.samplerates[i].max = device->sample_rates[i].max;
				}
			}
		};
	}

	uint32 speakerListClass::deviceCount() const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return numeric_cast<uint32>(impl->infos.size());
	}

	uint32 speakerListClass::deviceDefault() const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->defaultDevice;
	}

	string speakerListClass::deviceId(uint32 device) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].deviceId;
	}

	string speakerListClass::deviceName(uint32 device) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].deviceName;
	}

	bool speakerListClass::deviceRaw(uint32 device) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].raw;
	}

	uint32 speakerListClass::layoutCount(uint32 device) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return numeric_cast<uint32>(impl->infos[device].layouts.size());
	}

	uint32 speakerListClass::layoutCurrent(uint32 device) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].layoutCurrent;
	}

	string speakerListClass::layoutName(uint32 device, uint32 layout) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].layouts[layout].name;
	}

	uint32 speakerListClass::layoutChannels(uint32 device, uint32 layout) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return numeric_cast<uint32>(impl->infos[device].layouts[layout].channels.size());
	}

	uint32 speakerListClass::samplerateCount(uint32 device) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return numeric_cast<uint32>(impl->infos[device].samplerates.size());
	}

	uint32 speakerListClass::samplerateCurrent(uint32 device) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].samplerateCurrent;
	}

	uint32 speakerListClass::samplerateMin(uint32 device, uint32 samplerate) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].samplerates[samplerate].min;
	}

	uint32 speakerListClass::samplerateMax(uint32 device, uint32 samplerate) const
	{
		speakerListImpl *impl = (speakerListImpl*)this;
		return impl->infos[device].samplerates[samplerate].max;
	}

	holder<speakerListClass> newSpeakerList(bool inputs)
	{
		return detail::systemArena().createImpl <speakerListClass, speakerListImpl>(inputs);
	}
}