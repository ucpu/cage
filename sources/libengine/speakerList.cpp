#include <cage-core/pointerRangeHolder.h>

#include <cage-engine/sound.h>
#include <cage-engine/speakerList.h>
#include "sound/private.h"

#include <vector>

namespace cage
{
	SpeakerLayout::SpeakerLayout() : channels(0)
	{}

	SpeakerSamplerate::SpeakerSamplerate() : minimum(0), maximum(0)
	{}

	namespace
	{
		struct SpeakerDeviceImpl : public SpeakerDevice
		{
			string deviceId;
			string deviceName;
			bool raw;

			std::vector<SpeakerLayout> layouts;
			uint32 layoutCurrent;

			std::vector<SpeakerSamplerate> samplerates;
			uint32 samplerateCurrent;

			string formatCurrent;
			real latencyMin, latencyMax, latencyCurrent;

			SpeakerDeviceImpl(SoundIoDevice *device) : raw(false), layoutCurrent(m), samplerateCurrent(0)
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

		class SpeakerListImpl : public SpeakerList
		{
		public:
			std::vector<Holder<SpeakerDeviceImpl>> devices;
			uint32 defaultDevice;

			SpeakerListImpl(bool inputs) : defaultDevice(m)
			{
				Holder<SoundContext> cnx = newSoundContext(SoundContextCreateConfig());
				SoundIo *soundio = soundioFromContext(cnx.get());
				if (inputs)
				{
					uint32 count = soundio_input_device_count(soundio);
					devices.reserve(count);
					for (uint32 i = 0; i < count; i++)
					{
						SoundIoDevice *device = soundio_get_input_device(soundio, i);
						devices.push_back(detail::systemArena().createHolder<SpeakerDeviceImpl>(device));
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
						devices.push_back(detail::systemArena().createHolder<SpeakerDeviceImpl>(device));
						soundio_device_unref(device);
					}
					defaultDevice = soundio_default_output_device_index(soundio);
				}
			}
		};
	}

	string SpeakerDevice::id() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->deviceId;
	}

	string SpeakerDevice::name() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->deviceName;
	}

	bool SpeakerDevice::raw() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->raw;
	}

	uint32 SpeakerDevice::layoutsCount() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return numeric_cast<uint32>(impl->layouts.size());
	}

	uint32 SpeakerDevice::currentLayout() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->layoutCurrent;
	}

	const SpeakerLayout &SpeakerDevice::layout(uint32 index) const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->layouts[index];
	}

	PointerRange<const SpeakerLayout> SpeakerDevice::layouts() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->layouts;
	}

	uint32 SpeakerDevice::sampleratesCount() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return numeric_cast<uint32>(impl->samplerates.size());
	}

	uint32 SpeakerDevice::currentSamplerate() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->samplerateCurrent;
	}

	const SpeakerSamplerate &SpeakerDevice::samplerate(uint32 index) const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->samplerates[index];
	}

	PointerRange<const SpeakerSamplerate> SpeakerDevice::samplerates() const
	{
		const SpeakerDeviceImpl *impl = (const SpeakerDeviceImpl *)this;
		return impl->samplerates;
	}


	uint32 SpeakerList::devicesCount() const
	{
		const SpeakerListImpl *impl = (const SpeakerListImpl *)this;
		return numeric_cast<uint32>(impl->devices.size());
	}

	uint32 SpeakerList::defaultDevice() const
	{
		const SpeakerListImpl *impl = (const SpeakerListImpl *)this;
		return impl->defaultDevice;
	}

	const SpeakerDevice *SpeakerList::device(uint32 index) const
	{
		const SpeakerListImpl *impl = (const SpeakerListImpl *)this;
		return impl->devices[index].get();
	}

	Holder<PointerRange<const SpeakerDevice *>> SpeakerList::devices() const
	{
		const SpeakerListImpl *impl = (const SpeakerListImpl *)this;
		PointerRangeHolder<const SpeakerDevice *> prh;
		prh.reserve(impl->devices.size());
		for (auto &it : impl->devices)
			prh.push_back(it.get());
		return prh;
	}

	Holder<SpeakerList> newSpeakerList(bool inputs)
	{
		return detail::systemArena().createImpl<SpeakerList, SpeakerListImpl>(inputs);
	}
}
