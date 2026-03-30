#include <GLFW/glfw3.h>

#include <cage-engine/core.h>

namespace cage
{
	class Mutex;
	class Window;
	class Cursor;

	namespace privat
	{
		struct GraphicsContext;

		Holder<GraphicsContext> &getGraphicsContext(Window *w);
		void glfwInitializeFunc();
		void glfwInitializeGamepads();
		Mutex *glfwMutex();
		void glfwPokeCursor(Window *w);
		String getMonitorId(GLFWmonitor *monitor);
		GLFWmonitor *getMonitorById(const String &id);
		GLFWwindow *getGlfwWindow(Window *w);
		GLFWcursor *getCursor(Cursor *c);
	}
}
