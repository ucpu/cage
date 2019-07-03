#include <cage-core/core.h>
#include <cage-core/pointerRangeHolder.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include "graphics/private.h" // getMonitorId
#include <cage-client/screenList.h>
#include <GLFW/glfw3.h>

#include <vector>

namespace cage
{
	void cageGlfwInitializeFunc();

	namespace
	{
		bool operator == (const GLFWvidmode &a, const GLFWvidmode &b)
		{
			return a.width == b.width && a.height == b.height && a.refreshRate == b.refreshRate;
		}

		class screenDeviceImpl : public screenDevice
		{
		public:
			screenDeviceImpl(GLFWmonitor *m) : name_(glfwGetMonitorName(m)), id_(getMonitorId(m)), current_(cage::m)
			{
				int cnt = 0;
				const GLFWvidmode *ms = glfwGetVideoModes(m, &cnt);
				const GLFWvidmode *cur = glfwGetVideoMode(m);
				modes.reserve(cnt);
				for (uint32 i = 0; i < numeric_cast<uint32>(cnt); i++)
				{
					screenMode m;
					m.frequency = ms[i].refreshRate;
					m.resolution.x = ms[i].width;
					m.resolution.y = ms[i].height;
					modes.push_back(m);
					if (ms[i] == *cur)
						current_ = i;
				}
			}

			const string name_;
			const string id_;
			std::vector<screenMode> modes;
			uint32 current_;
		};

		class screenListImpl : public screenList
		{
		public:
			screenListImpl() : primary(m)
			{
				cageGlfwInitializeFunc();
				int cnt = 0;
				GLFWmonitor **ms = glfwGetMonitors(&cnt);
				devices.reserve(cnt);
				GLFWmonitor *prim = glfwGetPrimaryMonitor();
				for (uint32 i = 0; i < numeric_cast<uint32>(cnt); i++)
				{
					if (ms[i] == prim)
						primary = i;
					devices.push_back(detail::systemArena().createHolder<screenDeviceImpl>(ms[i]));
				}
			}

			std::vector<holder<screenDeviceImpl>> devices;
			uint32 primary;
		};
	}

	screenMode::screenMode() : frequency(0)
	{}

	uint32 screenDevice::modesCount() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return numeric_cast<uint32>(impl->modes.size());
	}

	uint32 screenDevice::currentMode() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->current_;
	}

	const screenMode &screenDevice::mode(uint32 index) const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->modes[index];
	}

	pointerRange<const screenMode> screenDevice::modes() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->modes;
	}

	string screenDevice::name() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->name_;
	}

	string screenDevice::id() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->id_;
	}

	uint32 screenList::devicesCount() const
	{
		screenListImpl *impl = (screenListImpl*)this;
		return numeric_cast<uint32>(impl->devices.size());
	}

	uint32 screenList::defaultDevice() const
	{
		screenListImpl *impl = (screenListImpl*)this;
		return impl->primary;
	}

	const screenDevice *screenList::device(uint32 index) const
	{
		screenListImpl *impl = (screenListImpl*)this;
		return impl->devices[index].get();
	}

	holder<pointerRange<const screenDevice*>> screenList::devices() const
	{
		const screenListImpl *impl = (const screenListImpl*)this;
		pointerRangeHolder<const screenDevice*> prh;
		prh.reserve(impl->devices.size());
		for (auto &it : impl->devices)
			prh.push_back(it.get());
		return prh;
	}

	holder<screenList> newScreenList()
	{
		return detail::systemArena().createImpl<screenList, screenListImpl>();
	}
}
