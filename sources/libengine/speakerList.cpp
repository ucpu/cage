#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/pointerRangeHolder.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/sound.h>
#include <cage-engine/speakerList.h>
#include "sound/private.h"

#include <vector>

namespace cage
{
	speakerLayout::speakerLayout() : channels(0)
	{}

	speakerSamplerate::speakerSamplerate() : minimum(0), maximum(0)
	{}

	namespace
	{
		struct speakerDeviceImpl : public speakerDevice
		{
			string deviceId;
			string deviceName;
			bool raw;

			std::vector<speakerLayout> layouts;
			uint32 layoutCurrent;

			std::vector<speakerSamplerate> samplerates;
			uint32 samplerateCurrent;

			string formatCurrent;
			real latencyMin, latencyMax, latencyCurrent;

			speakerDeviceImpl(SoundIoDevice *device) : raw(false), layoutCurrent(m), samplerateCurrent(0)
			{
				// device
				deviceId = device->id;
				deviceName = device->name;
				raw = device->is_raw;
				formatCurrent = soundio_format_string(device->current_format);

				// layouts
				uint32 lc = device->layout_count;
				layouts.resize(lc);
				for (uint32 i = 0; i < lc; i++)
				{
					layouts[i].name = device->layouts[i].name;
					layouts[i].channels = device->layouts[i].channel_count;
					if (soundio_channel_layout_equal(&device->layouts[i], &device->current_layout))
						layoutCurrent = i;
				}

				// sample rates
				uint32 sc = device->sample_rate_count;
				samplerates.resize(sc);
				for (uint32 i = 0; i < sc; i++)
				{
					samplerates[i].minimum = device->sample_rates[i].min;
					samplerates[i].maximum = device->sample_rates[i].max;
				}
				samplerateCurrent = device->sample_rate_current;
			}
		};

		class speakerListImpl : public speakerList
		{
		public:
			std::vector<Holder<speakerDeviceImpl>> devices;
			uint32 defaultDevice;

			speakerListImpl(bool inputs) : defaultDevice(m)
			{
				Holder<soundContext> cnx = newSoundContext(soundContextCreateConfig());
				SoundIo *soundio = soundioFromContext(cnx.get());
				if (inputs)
				{
					uint32 count = soundio_input_device_count(soundio);
					devices.reserve(count);
					for (uint32 i = 0; i < count; i++)
					{
						SoundIoDevice *device = soundio_get_input_device(soundio, i);
						devices.push_back(detail::systemArena().createHolder<speakerDeviceImpl>(device));
						soundio_device_unref(device);
					}
					defaultDevice = soundio_default_input_device_index(soundio);
				}
				else
				{
					int count = soundio_output_device_count(soundio);
					devices.reserve(count);
					for (int i = 0; i < count; i++)
					{
						SoundIoDevice *device = soundio_get_output_device(soundio, i);
						devices.push_back(detail::systemArena().createHolder<speakerDeviceImpl>(device));
						soundio_device_unref(device);
					}
					defaultDevice = soundio_default_output_device_index(soundio);
				}
			}
		};
	}

	string speakerDevice::id() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->deviceId;
	}

	string speakerDevice::name() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->deviceName;
	}

	bool speakerDevice::raw() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->raw;
	}

	uint32 speakerDevice::layoutsCount() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return numeric_cast<uint32>(impl->layouts.size());
	}

	uint32 speakerDevice::currentLayout() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->layoutCurrent;
	}

	const speakerLayout &speakerDevice::layout(uint32 index) const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->layouts[index];
	}

	PointerRange<const speakerLayout> speakerDevice::layouts() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->layouts;
	}

	uint32 speakerDevice::sampleratesCount() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return numeric_cast<uint32>(impl->samplerates.size());
	}

	uint32 speakerDevice::currentSamplerate() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->samplerateCurrent;
	}

	const speakerSamplerate &speakerDevice::samplerate(uint32 index) const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->samplerates[index];
	}

	PointerRange<const speakerSamplerate> speakerDevice::samplerates() const
	{
		const speakerDeviceImpl *impl = (const speakerDeviceImpl *)this;
		return impl->samplerates;
	}


	uint32 speakerList::devicesCount() const
	{
		const speakerListImpl *impl = (const speakerListImpl *)this;
		return numeric_cast<uint32>(impl->devices.size());
	}

	uint32 speakerList::defaultDevice() const
	{
		const speakerListImpl *impl = (const speakerListImpl *)this;
		return impl->defaultDevice;
	}

	const speakerDevice *speakerList::device(uint32 index) const
	{
		const speakerListImpl *impl = (const speakerListImpl *)this;
		return impl->devices[index].get();
	}

	Holder<PointerRange<const speakerDevice *>> speakerList::devices() const
	{
		const speakerListImpl *impl = (const speakerListImpl *)this;
		PointerRangeHolder<const speakerDevice *> prh;
		prh.reserve(impl->devices.size());
		for (auto &it : impl->devices)
			prh.push_back(it.get());
		return prh;
	}

	Holder<speakerList> newSpeakerList(bool inputs)
	{
		return detail::systemArena().createImpl<speakerList, speakerListImpl>(inputs);
	}
}
