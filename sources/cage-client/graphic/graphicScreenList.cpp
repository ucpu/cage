#include <vector>

#include <cage-core/core.h>
#include <cage-core/math.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include "private.h"
#include <GLFW/glfw3.h>

namespace cage
{
	namespace
	{
		bool operator == (const GLFWvidmode &a, const GLFWvidmode &b)
		{
			return a.width == b.width && a.height == b.height && a.refreshRate == b.refreshRate;
		}

		class screenDeviceImpl : public screenDeviceClass
		{
		public:
			screenDeviceImpl(GLFWmonitor *m) : name_(glfwGetMonitorName(m)), id_(getMonitorId(m)), current_(-1)
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

		class screenListImpl : public screenListClass
		{
		public:
			screenListImpl() : primary(-1)
			{
				int cnt = 0;
				GLFWmonitor **ms = glfwGetMonitors(&cnt);
				devices.reserve(cnt);
				GLFWmonitor *prim = glfwGetPrimaryMonitor();
				for (uint32 i = 0; i < numeric_cast<uint32>(cnt); i++)
				{
					if (ms[i] == prim)
						primary = i;
					devices.push_back(screenDeviceImpl(ms[i]));
				}
			}

			std::vector<screenDeviceImpl> devices;
			uint32 primary;
		};
	}

	screenMode::screenMode() : frequency(0)
	{}

	uint32 screenDeviceClass::modesCount() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return numeric_cast<uint32>(impl->modes.size());
	}

	uint32 screenDeviceClass::currentMode() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->current_;
	}

	const screenMode &screenDeviceClass::mode(uint32 index) const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->modes[index];
	}

	pointerRange<const screenMode> screenDeviceClass::modes() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return { impl->modes.data(), impl->modes.data() + impl->modes.size() };
	}

	string screenDeviceClass::name() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->name_;
	}

	string screenDeviceClass::id() const
	{
		screenDeviceImpl *impl = (screenDeviceImpl*)this;
		return impl->id_;
	}

	uint32 screenListClass::devicesCount() const
	{
		screenListImpl *impl = (screenListImpl*)this;
		return numeric_cast<uint32>(impl->devices.size());
	}

	uint32 screenListClass::primaryDevice() const
	{
		screenListImpl *impl = (screenListImpl*)this;
		return impl->primary;
	}

	const screenDeviceClass &screenListClass::device(uint32 index) const
	{
		screenListImpl *impl = (screenListImpl*)this;
		return impl->devices[index];
	}

	pointerRange<const screenDeviceClass> screenListClass::devices() const
	{
		screenListImpl *impl = (screenListImpl*)this;
		return { impl->devices.data(), impl->devices.data() + impl->devices.size() };
	}

	holder<screenListClass> newScreenList()
	{
		return detail::systemArena().createImpl <screenListClass, screenListImpl>();
	}
}