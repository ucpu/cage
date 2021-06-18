#include <cage-core/pointerRangeHolder.h>

#include <cage-engine/screenList.h>
#include "graphics/private.h" // getMonitorId

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

		class ScreenDeviceImpl : public ScreenDevice
		{
		public:
			ScreenDeviceImpl(GLFWmonitor *m) : name_(glfwGetMonitorName(m)), id_(getMonitorId(m)), current_(cage::m)
			{
				int cnt = 0;
				const GLFWvidmode *ms = glfwGetVideoModes(m, &cnt);
				const GLFWvidmode *cur = glfwGetVideoMode(m);
				modes.reserve(cnt);
				for (uint32 i = 0; i < numeric_cast<uint32>(cnt); i++)
				{
					ScreenMode m;
					m.frequency = ms[i].refreshRate;
					m.resolution[0] = ms[i].width;
					m.resolution[1] = ms[i].height;
					modes.push_back(m);
					if (ms[i] == *cur)
						current_ = i;
				}
			}

			const string name_;
			const string id_;
			std::vector<ScreenMode> modes;
			uint32 current_;
		};

		class ScreenListImpl : public ScreenList
		{
		public:
			ScreenListImpl() : primary(m)
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
					devices.push_back(systemMemory().createHolder<ScreenDeviceImpl>(ms[i]));
				}
			}

			std::vector<Holder<ScreenDeviceImpl>> devices;
			uint32 primary;
		};
	}

	uint32 ScreenDevice::currentMode() const
	{
		ScreenDeviceImpl *impl = (ScreenDeviceImpl*)this;
		return impl->current_;
	}

	PointerRange<const ScreenMode> ScreenDevice::modes() const
	{
		ScreenDeviceImpl *impl = (ScreenDeviceImpl*)this;
		return impl->modes;
	}

	string ScreenDevice::name() const
	{
		ScreenDeviceImpl *impl = (ScreenDeviceImpl*)this;
		return impl->name_;
	}

	string ScreenDevice::id() const
	{
		ScreenDeviceImpl *impl = (ScreenDeviceImpl*)this;
		return impl->id_;
	}

	uint32 ScreenList::defaultDevice() const
	{
		ScreenListImpl *impl = (ScreenListImpl*)this;
		return impl->primary;
	}

	Holder<PointerRange<const ScreenDevice*>> ScreenList::devices() const
	{
		const ScreenListImpl *impl = (const ScreenListImpl*)this;
		PointerRangeHolder<const ScreenDevice*> prh;
		prh.reserve(impl->devices.size());
		for (auto &it : impl->devices)
			prh.push_back(it.get());
		return prh;
	}

	Holder<ScreenList> newScreenList()
	{
		return systemMemory().createImpl<ScreenList, ScreenListImpl>();
	}
}
