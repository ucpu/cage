#include <cage-engine/core.h>

#include <GLFW/glfw3.h>

namespace cage
{
	void cageGlfwInitializeFunc();
	Mutex *cageGlfwMutex();
	String getMonitorId(GLFWmonitor *monitor);
	GLFWmonitor *getMonitorById(const String &id);
}
