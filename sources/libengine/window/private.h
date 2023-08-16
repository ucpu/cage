#include <GLFW/glfw3.h>

#include <cage-engine/core.h>

namespace cage
{
	void cageGlfwInitializeFunc();
	class Mutex *cageGlfwMutex();
	String getMonitorId(GLFWmonitor *monitor);
	GLFWmonitor *getMonitorById(const String &id);
}
