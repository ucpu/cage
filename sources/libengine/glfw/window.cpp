#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/config.h>
#include <cage-core/flatSet.h>
#include <cage-engine/window.h>
#include <cage-engine/opengl.h>
#include "private.h"
#include "../graphics/private.h"

#include <atomic>
#include <vector>

#ifdef CAGE_SYSTEM_WINDOWS
#define GCHL_WINDOWS_THREAD
#endif // CAGE_SYSTEM_WINDOWS

namespace cage
{
	namespace graphicsPrivat
	{
		void openglContextInitializeGeneral(Window *w);
	}

	namespace
	{
		ConfigBool confDebugContext("cage/graphics/debugContext",
#ifdef CAGE_DEBUG
			true
#else
			false
#endif // CAGE_DEBUG
		);
		ConfigBool confNoErrorContext("cage/graphics/noErrorContext", false);

		ModifiersFlags getKeyModifiers(int mods)
		{
			return ModifiersFlags::None
				| ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT ? ModifiersFlags::Shift : ModifiersFlags::None)
				| ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL ? ModifiersFlags::Ctrl : ModifiersFlags::None)
				| ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT ? ModifiersFlags::Alt : ModifiersFlags::None)
				| ((mods & GLFW_MOD_SUPER) == GLFW_MOD_SUPER ? ModifiersFlags::Super : ModifiersFlags::None);
		}

		ModifiersFlags getKeyModifiers(GLFWwindow *w)
		{
			ModifiersFlags r = ModifiersFlags::None;
			if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
				r |= ModifiersFlags::Shift;
			if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
				r |= ModifiersFlags::Ctrl;
			if (glfwGetKey(w, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
				r |= ModifiersFlags::Alt;
			if (glfwGetKey(w, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
				r |= ModifiersFlags::Super;
			return r;
		}

		struct MouseOffset
		{
			Vec2i off;
		};

		class WindowImpl : public Window
		{
		public:
			uint64 lastMouseButtonPressTimes[5] = {}; // unused, left, right, unused, middle
			ConcurrentQueue<GenericInput> eventsQueue;
			FlatSet<uint32> stateKeys;
			Vec2i stateMousePosition;
			MouseButtonsFlags stateButtons = MouseButtonsFlags::None;
			ModifiersFlags stateMods = ModifiersFlags::None;
			Window *shareContext = nullptr;
			GLFWwindow *window = nullptr;
			bool focus = true;

#ifdef GCHL_WINDOWS_THREAD
			Holder<Thread> windowThread;
			Holder<Semaphore> windowSemaphore;
			std::atomic<bool> stopping = false;
			Vec2i mouseOffsetApi;
			ConcurrentQueue<Vec2i> mouseOffsetsThr;

			void threadEntry()
			{
				try
				{
					initializeWindow();
				}
				catch (...)
				{
					stopping = true;
				}
				windowSemaphore->unlock();
				while (!stopping)
				{
					{
						ScopeLock l(cageGlfwMutex());
						glfwPollEvents();
					}

					Vec2i d;
					while (mouseOffsetsThr.tryPop(d))
					{
						double x, y;
						glfwGetCursorPos(window, &x, &y);
						x += d[0];
						y += d[1];
						glfwSetCursorPos(window, x, y);
						MouseOffset off;
						off.off = -d;
						GenericInput e;
						e.type = InputClassEnum::Custom;
						e.data = off;
						eventsQueue.push(e);
					}

					threadSleep(5000);
				}
				finalizeWindow();
			}
#endif

			explicit WindowImpl(Window *shareContext) : shareContext(shareContext)
			{
				ScopeLock l(cageGlfwMutex());

#ifdef GCHL_WINDOWS_THREAD
				windowSemaphore = newSemaphore(0, 1);
				static uint32 threadIndex = 0;
				windowThread = newThread(Delegate<void()>().bind<WindowImpl, &WindowImpl::threadEntry>(this), Stringizer() + "window " + (threadIndex++));
				windowSemaphore->lock();
				windowSemaphore.clear();
				if (stopping)
					CAGE_THROW_ERROR(Exception, "failed to initialize window");
#else
				initializeWindow();
#endif

				// initialize opengl
				makeCurrent();
				if (!gladLoadGL())
					CAGE_THROW_ERROR(Exception, "gladLoadGl");
				openglContextInitializeGeneral(this);
			}

			~WindowImpl()
			{
				makeNotCurrent();
#ifdef GCHL_WINDOWS_THREAD
				stopping = true;
				if (windowThread)
					windowThread->wait();
#else
				finalizeWindow();
#endif
			}

			void initializeWindow()
			{
				glfwDefaultWindowHints();
				glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
				glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
				glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
				glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
				glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, confDebugContext ? GLFW_TRUE : GLFW_FALSE);
				if (confDebugContext)
					CAGE_LOG(SeverityEnum::Info, "graphics", "creating DEBUG opengl context");
				bool noErr = confNoErrorContext && !confDebugContext;
				glfwWindowHint(GLFW_CONTEXT_NO_ERROR, noErr ? GLFW_TRUE : GLFW_FALSE);
				if (noErr)
					CAGE_LOG(SeverityEnum::Info, "graphics", "creating NO-ERROR opengl context");
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				glfwWindowHint(GLFW_ALPHA_BITS, 0); // save some gpu memory
				glfwWindowHint(GLFW_DEPTH_BITS, 0);
				glfwWindowHint(GLFW_STENCIL_BITS, 0);
				window = glfwCreateWindow(1, 1, "Cage Window", NULL, shareContext ? ((WindowImpl *)shareContext)->window : NULL);
				if (!window)
					CAGE_THROW_ERROR(Exception, "failed to create window");
				glfwSetWindowUserPointer(window, this);
				glfwSetWindowSizeLimits(window, 160, 120, GLFW_DONT_CARE, GLFW_DONT_CARE);
				initializeEvents();
			}

			void finalizeWindow()
			{
				ScopeLock lock(cageGlfwMutex());
				glfwDestroyWindow(window);
				window = nullptr;
			}

			void initializeEvents();

			bool determineMouseDoubleClick(InputMouse in)
			{
				CAGE_ASSERT((uint32)in.buttons < sizeof(lastMouseButtonPressTimes) / sizeof(lastMouseButtonPressTimes[0]));
				const uint64 current = applicationTime();
				uint64 &last = lastMouseButtonPressTimes[(uint32)in.buttons];
				if (current - last < 500000)
				{
					last = 0;
					return true;
				}
				else
				{
					last = current;
					return false;
				}
			}

			Vec2i offsetMousePositionApi(Vec2i p)
			{
				stateMousePosition = p;
#ifdef GCHL_WINDOWS_THREAD
				stateMousePosition += mouseOffsetApi;
#endif
				return stateMousePosition;
			}

			void offsetMousePositionApi(GenericInput in)
			{
				if (in.type == InputClassEnum::MouseWheel)
				{
					InputMouseWheel i = in.data.get<InputMouseWheel>();
					i.position = offsetMousePositionApi(i.position);
					in.data = i;
				}
				else
				{
					InputMouse i = in.data.get<InputMouse>();
					i.position = offsetMousePositionApi(i.position);
					in.data = i;
				}
				events.dispatch(in);
			}
		};

		void windowCloseEvent(GLFWwindow *w)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputWindow e;
			e.window = impl;
			impl->eventsQueue.push({ e, InputClassEnum::WindowClose });
		}

		void windowKeyCallback(GLFWwindow *w, int key, int, int action, int mods)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputClassEnum type;
			switch (action)
			{
			case GLFW_PRESS:
				type = InputClassEnum::KeyPress;
				break;
			case GLFW_REPEAT:
				type = InputClassEnum::KeyRepeat;
				break;
			case GLFW_RELEASE:
				type = InputClassEnum::KeyRelease;
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid key action");
			}
			InputKey e;
			e.window = impl;
			e.mods = getKeyModifiers(mods);
			e.key = key;
			impl->eventsQueue.push({ e, type });
		}

		void windowCharCallback(GLFWwindow *w, unsigned int codepoint)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputKey e;
			e.window = impl;
			e.mods = getKeyModifiers(w);
			e.key = codepoint;
			impl->eventsQueue.push({ e, InputClassEnum::KeyChar });
		}

		void windowMouseMove(GLFWwindow *w, double xpos, double ypos)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputMouse e;
			e.window = impl;
			e.mods = getKeyModifiers(w);
			e.position[0] = numeric_cast<sint32>(floor(xpos));
			e.position[1] = numeric_cast<sint32>(floor(ypos));
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
				e.buttons |= MouseButtonsFlags::Left;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT))
				e.buttons |= MouseButtonsFlags::Right;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE))
				e.buttons |= MouseButtonsFlags::Middle;
			impl->eventsQueue.push({ e, InputClassEnum::MouseMove });
		}

		void windowMouseButtonCallback(GLFWwindow *w, int button, int action, int mods)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputClassEnum type;
			switch (action)
			{
			case GLFW_PRESS:
				type = InputClassEnum::MousePress;
				break;
			case GLFW_RELEASE:
				type = InputClassEnum::MouseRelease;
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid mouse action");
			}
			InputMouse e;
			e.window = impl;
			e.mods = getKeyModifiers(mods);
			switch (button)
			{
			case GLFW_MOUSE_BUTTON_LEFT:
				e.buttons = MouseButtonsFlags::Left;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				e.buttons = MouseButtonsFlags::Right;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				e.buttons = MouseButtonsFlags::Middle;
				break;
			default:
				return; // ignore other keys
			}
			double xpos, ypos;
			glfwGetCursorPos(w, &xpos, &ypos);
			e.position[0] = numeric_cast<sint32>(floor(xpos));
			e.position[1] = numeric_cast<sint32>(floor(ypos));
			const bool doubleClick = type == InputClassEnum::MousePress && impl->determineMouseDoubleClick(e);
			impl->eventsQueue.push({ e, type });
			if (doubleClick)
				impl->eventsQueue.push({ e, InputClassEnum::MouseDoublePress });
		}

		void windowMouseWheel(GLFWwindow *w, double, double yoffset)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputMouseWheel e;
			e.window = impl;
			e.mods = getKeyModifiers(w);
			double xpos, ypos;
			glfwGetCursorPos(w, &xpos, &ypos);
			e.position[0] = numeric_cast<sint32>(floor(xpos));
			e.position[1] = numeric_cast<sint32>(floor(ypos));
			e.wheel = numeric_cast<sint32>(yoffset);
			impl->eventsQueue.push({ e, InputClassEnum::MouseWheel });
		}

		void windowResizeCallback(GLFWwindow *w, int width, int height)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputWindowValue e;
			e.window = impl;
			e.value = Vec2i(width, height);
			impl->eventsQueue.push({ e, InputClassEnum::WindowResize });
		}

		void windowMoveCallback(GLFWwindow *w, int xpos, int ypos)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputWindowValue e;
			e.window = impl;
			e.value = Vec2i(xpos, ypos);
			impl->eventsQueue.push({ e, InputClassEnum::WindowMove });
		}

		void windowIconifiedCallback(GLFWwindow *w, int iconified)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputWindow e;
			e.window = impl;
			impl->eventsQueue.push({ e, iconified ? InputClassEnum::WindowHide : InputClassEnum::WindowShow });
		}

		void windowMaximizedCallback(GLFWwindow *w, int maximized)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputWindow e;
			e.window = impl;
			impl->eventsQueue.push({ e, InputClassEnum::WindowShow }); // window is visible when both maximized or restored
		}

		void windowFocusCallback(GLFWwindow *w, int focused)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			InputWindow e;
			e.window = impl;
			impl->eventsQueue.push({ e, focused ? InputClassEnum::FocusGain : InputClassEnum::FocusLose });
		}

		void WindowImpl::initializeEvents()
		{
			glfwSetWindowCloseCallback(window, &windowCloseEvent);
			glfwSetKeyCallback(window, &windowKeyCallback);
			glfwSetCharCallback(window, &windowCharCallback);
			glfwSetCursorPosCallback(window, &windowMouseMove);
			glfwSetMouseButtonCallback(window, &windowMouseButtonCallback);
			glfwSetScrollCallback(window, &windowMouseWheel);
			glfwSetWindowSizeCallback(window, &windowResizeCallback);
			glfwSetWindowPosCallback(window, &windowMoveCallback);
			glfwSetWindowIconifyCallback(window, &windowIconifiedCallback);
			glfwSetWindowMaximizeCallback(window, &windowMaximizedCallback);
			glfwSetWindowFocusCallback(window, &windowFocusCallback);
		}
	}

	void Window::title(const String &value)
	{
		WindowImpl *impl = (WindowImpl *)this;
		glfwSetWindowTitle(impl->window, value.c_str());
	}

	bool Window::isFocused() const
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifdef GCHL_WINDOWS_THREAD
		return impl->focus;
#else
		return glfwGetWindowAttrib(impl->window, GLFW_FOCUSED);
#endif
	}

	bool Window::isFullscreen() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return !!glfwGetWindowMonitor(impl->window);
	}

	bool Window::isMaximized() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return !!glfwGetWindowAttrib(impl->window, GLFW_MAXIMIZED);
	}

	bool Window::isWindowed() const
	{
		return !isHidden() && !isMinimized() && !isMaximized() && !isFullscreen();
	}

	bool Window::isMinimized() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return !!glfwGetWindowAttrib(impl->window, GLFW_ICONIFIED);
	}

	bool Window::isHidden() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return !glfwGetWindowAttrib(impl->window, GLFW_VISIBLE);
	}

	bool Window::isVisible() const
	{
		return !isHidden() && !isMinimized();
	}

	namespace
	{
		void normalizeWindow(WindowImpl *impl, WindowFlags flags)
		{
			if (impl->isFullscreen())
				glfwSetWindowMonitor(impl->window, nullptr, 100, 100, 800, 600, 0);
			if (impl->isHidden())
				glfwShowWindow(impl->window);
			if (impl->isMinimized() || impl->isMaximized())
				glfwRestoreWindow(impl->window);
			glfwSetWindowAttrib(impl->window, GLFW_DECORATED, any(flags & WindowFlags::Border));
			glfwSetWindowAttrib(impl->window, GLFW_RESIZABLE, any(flags & WindowFlags::Resizeable));
		}
	}

	void Window::setFullscreen(const Vec2i &resolution, uint32 frequency, const String &deviceId)
	{
		WindowImpl *impl = (WindowImpl *)this;
		normalizeWindow(impl, WindowFlags::None);
		GLFWmonitor *m = getMonitorById(deviceId);
		const GLFWvidmode *v = glfwGetVideoMode(m);
		glfwSetWindowMonitor(impl->window, m, 0, 0,
			resolution[0] == 0 ? v->width : resolution[0],
			resolution[1] == 0 ? v->height : resolution[1],
			frequency == 0 ? glfwGetVideoMode(m)->refreshRate : frequency);
	}

	void Window::setMaximized()
	{
		WindowImpl *impl = (WindowImpl *)this;
		normalizeWindow(impl, WindowFlags::Border | WindowFlags::Resizeable);
		glfwMaximizeWindow(impl->window);
	}

	void Window::setWindowed(WindowFlags flags)
	{
		WindowImpl *impl = (WindowImpl *)this;
		normalizeWindow(impl, flags);
	}

	void Window::setMinimized()
	{
		WindowImpl *impl = (WindowImpl *)this;
		normalizeWindow(impl, WindowFlags::Resizeable);
		glfwIconifyWindow(impl->window);
	}

	void Window::setHidden()
	{
		WindowImpl *impl = (WindowImpl *)this;
		normalizeWindow(impl, WindowFlags::None);
		glfwHideWindow(impl->window);
	}

	bool Window::mouseVisible() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return glfwGetInputMode(impl->window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
	}

	void Window::mouseVisible(bool value)
	{
		WindowImpl *impl = (WindowImpl *)this;
		if (value)
			glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	Vec2i Window::mousePosition() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return impl->stateMousePosition;
	}

	void Window::mousePosition(const Vec2i &p)
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifdef GCHL_WINDOWS_THREAD
		Vec2i d = p - impl->stateMousePosition;
		impl->stateMousePosition = p;
		impl->mouseOffsetApi += d;
		impl->mouseOffsetsThr.push(d);
#else
		glfwSetCursorPos(impl->window, p[0], p[1]);
#endif // GCHL_WINDOWS_THREAD
	}

	MouseButtonsFlags Window::mouseButtons() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return impl->stateButtons;
	}

	ModifiersFlags Window::keyboardModifiers() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return impl->stateMods;
	}

	bool Window::keyboardKey(uint32 key) const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return impl->stateKeys.count(key);
	}

	void Window::processEvents()
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifndef GCHL_WINDOWS_THREAD
		{
			ScopeLock l(cageGlfwMutex());
			glfwPollEvents();
		}
#endif
		GenericInput e;
		while (impl->eventsQueue.tryPop(e))
		{
			if (e.data.type() == detail::typeIndex<InputMouse>())
				impl->stateMods = e.data.get<InputMouse>().mods;
			if (e.data.type() == detail::typeIndex<InputMouseWheel>())
				impl->stateMods = e.data.get<InputMouseWheel>().mods;
			if (e.data.type() == detail::typeIndex<InputKey>())
				impl->stateMods = e.data.get<InputKey>().mods;

			switch (e.type)
			{
			case InputClassEnum::WindowClose:
			case InputClassEnum::WindowResize:
			case InputClassEnum::WindowMove:
			case InputClassEnum::WindowShow:
			case InputClassEnum::WindowHide:
			case InputClassEnum::KeyChar:
			case InputClassEnum::KeyRepeat:
				events.dispatch(e);
				break;
			case InputClassEnum::MouseMove:
			case InputClassEnum::MouseDoublePress:
			case InputClassEnum::MouseWheel:
				impl->offsetMousePositionApi(e);
				break;
			case InputClassEnum::KeyPress:
				impl->stateKeys.insert(e.data.get<InputKey>().key);
				events.dispatch(e);
				e.type = InputClassEnum::KeyRepeat;
				events.dispatch(e);
				break;
			case InputClassEnum::KeyRelease:
				events.dispatch(e);
				impl->stateKeys.erase(e.data.get<InputKey>().key);
				break;
			case InputClassEnum::MousePress:
				impl->stateButtons |= e.data.get<InputMouse>().buttons;
				impl->offsetMousePositionApi(e);
				break;
			case InputClassEnum::MouseRelease:
				impl->offsetMousePositionApi(e);
				impl->stateButtons &= ~e.data.get<InputMouse>().buttons;
				break;
			case InputClassEnum::FocusGain:
				impl->focus = true;
				events.dispatch(e);
				break;
			case InputClassEnum::FocusLose:
				impl->focus = false;
				events.dispatch(e);
				break;
#ifdef GCHL_WINDOWS_THREAD
			case InputClassEnum::Custom:
				impl->mouseOffsetApi += e.data.get<MouseOffset>().off;
				break;
#endif
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid window event type");
			}
		}
	}

	Vec2i Window::resolution() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		int w = 0, h = 0;
		glfwGetFramebufferSize(impl->window, &w, &h);
		return Vec2i(w, h);
	}

	Real Window::contentScaling() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		float x = 1, y = 1;
		glfwGetWindowContentScale(impl->window, &x, &y);
		return max(x, y);
	}

	String Window::screenId() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		{
			auto m = glfwGetWindowMonitor(impl->window);
			if (m)
				return getMonitorId(m);
		}
		const Vec2 center = Vec2(windowedPosition() + windowedSize() / 2);
		int cnt = 0;
		GLFWmonitor **ms = glfwGetMonitors(&cnt);
		// todo first filter out monitors that do not overlap with the center of the window
		uint32 bestIndex = m;
		Real bestDist = Real::Infinity();
		for (uint32 i = 0; i < numeric_cast<uint32>(cnt); i++)
		{
			int x, y, w, h;
			glfwGetMonitorWorkarea(ms[i], &x, &y, &w, &h);
			const Real dist = distance(center, Vec2(x, y) + Vec2(w, h) / 2);
			if (dist < bestDist)
			{
				bestDist = dist;
				bestIndex = i;
			}
		}
		CAGE_ASSERT(bestIndex != m);
		return getMonitorId(ms[bestIndex]);
	}

	Vec2i Window::windowedSize() const
	{
		WindowImpl *impl = (WindowImpl*)this;
		int w = 0, h = 0;
		glfwGetWindowSize(impl->window, &w, &h);
		return Vec2i(w, h);
	}

	void Window::windowedSize(const Vec2i &tmp)
	{
		WindowImpl *impl = (WindowImpl*)this;
		glfwSetWindowSize(impl->window, tmp[0], tmp[1]);
	}

	Vec2i Window::windowedPosition() const
	{
		WindowImpl *impl = (WindowImpl*)this;
		int x = 0, y = 0;
		glfwGetWindowPos(impl->window, &x, &y);
		return Vec2i(x, y);
	}

	void Window::windowedPosition(const Vec2i &tmp)
	{
		WindowImpl *impl = (WindowImpl*)this;
		glfwSetWindowPos(impl->window, tmp[0], tmp[1]);
	}

	void Window::makeCurrent()
	{
		WindowImpl *impl = (WindowImpl*)this;
		glfwMakeContextCurrent(impl->window);
		setCurrentContext(this);
	}

	void Window::makeNotCurrent()
	{
		glfwMakeContextCurrent(nullptr);
		setCurrentContext(nullptr);
	}

	void Window::swapBuffers()
	{
		WindowImpl *impl = (WindowImpl*)this;
		glfwSwapBuffers(impl->window);
	}

	Holder<Window> newWindow(Window *shareContext)
	{
		cageGlfwInitializeFunc();
		return systemMemory().createImpl<Window, WindowImpl>(shareContext);
	}
}
