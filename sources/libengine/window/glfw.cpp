#include <cstring>

#include "private.h"

#ifdef CAGE_SYSTEM_WINDOWS
	#define GLFW_EXPOSE_NATIVE_WIN32
#endif // CAGE_SYSTEM_WINDOWS
#include <GLFW/glfw3native.h>

#include <cage-core/concurrent.h>
#include <cage-engine/clipboard.h>

namespace cage
{
	void cageGlfwInitializeGamepads();

	namespace
	{
		void handleGlfwError(int, const char *message)
		{
			CAGE_LOG(SeverityEnum::Error, "glfw", message);
		}
	}

	void cageGlfwInitializeFunc()
	{
		static int init = []()
		{
			CAGE_LOG(SeverityEnum::Info, "glfw", Stringizer() + "initializing glfw");
			glfwSetErrorCallback(&handleGlfwError);
			if (!glfwInit())
				CAGE_THROW_ERROR(Exception, "failed to initialize glfw");
			cageGlfwInitializeGamepads();
			return 0;
		}();
		(void)init;
	}

	Mutex *cageGlfwMutex()
	{
		static Holder<Mutex> *mut = new Holder<Mutex>(newMutex()); // intentional leak
		return +*mut;
	}

	String getMonitorId(GLFWmonitor *monitor)
	{
#ifdef CAGE_SYSTEM_WINDOWS
		return glfwGetWin32Monitor(monitor);
#endif // CAGE_SYSTEM_WINDOWS

		// this fallback solution will work for as long as the monitor is valid
		return Stringizer() + monitor;
	}

	GLFWmonitor *getMonitorById(const String &id)
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

	void setClipboard(const String &str)
	{
		glfwSetClipboardString(nullptr, str.c_str());
	}

	String getClipboard()
	{
		if (const char *tmp = glfwGetClipboardString(nullptr))
		{
			const auto len = std::strlen(tmp);
			if (len < String::MaxLength)
				return String(PointerRange(tmp, tmp + len));
		}
		return {};
	}
}
