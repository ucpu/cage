#include <cage-core/core.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <GLFW/glfw3.h>

#ifdef CAGE_SYSTEM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif // CAGE_SYSTEM_WINDOWS
#include <GLFW/glfw3native.h>

namespace cage
{
	namespace graphicsPrivat
	{
		string getMonitorId(GLFWmonitor *monitor)
		{
#ifdef CAGE_SYSTEM_WINDOWS
			return glfwGetWin32Monitor(monitor);
#endif // CAGE_SYSTEM_WINDOWS

			// this fallback solution will work for as long as the monitor is valid
			return (uintPtr)monitor;
		}

		GLFWmonitor *getMonitorById(const string &id)
		{
			int cnt = 0;
			GLFWmonitor **ms = glfwGetMonitors(&cnt);
			for (uint32 i = 0; i < numeric_cast<uint32>(cnt); i++)
			{
				if (getMonitorId(ms[i]) == id)
					return ms[i];
			}
			return glfwGetPrimaryMonitor();
		}
	}
}
