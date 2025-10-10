#include <GLFW/glfw3.h>

#include <cage-engine/core.h>

namespace cage
{
	class Window;
	struct GraphicsContext;

	void cageGlfwInitializeFunc();
	class Mutex *cageGlfwMutex();
	String getMonitorId(GLFWmonitor *monitor);
	GLFWmonitor *getMonitorById(const String &id);
	GLFWwindow *getGlfwWindow(Window *w);
	Holder<GraphicsContext> &getGlfwContext(Window *w);
}
