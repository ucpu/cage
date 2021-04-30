#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/config.h>
#include <cage-core/flatSet.h>

#include <cage-engine/window.h>
#include <cage-engine/opengl.h>
#include "graphics/private.h"

#include <GLFW/glfw3.h>

#include <atomic>
#include <vector>

#ifdef CAGE_SYSTEM_WINDOWS
#define GCHL_WINDOWS_THREAD
#endif // CAGE_SYSTEM_WINDOWS

namespace cage
{
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

		void handleGlfwError(int, const char *message)
		{
			CAGE_LOG(SeverityEnum::Error, "glfw", message);
		}

		class CageGlfwInitializer
		{
		public:
			CageGlfwInitializer()
			{
				CAGE_LOG(SeverityEnum::Info, "glfw", stringizer() + "initializing glfw");
				glfwSetErrorCallback(&handleGlfwError);
				if (!glfwInit())
					CAGE_THROW_ERROR(Exception, "failed to initialize glfw");
			}

			~CageGlfwInitializer()
			{
				CAGE_LOG(SeverityEnum::Info, "glfw", stringizer() + "terminating glfw");
				glfwTerminate();
			}
		};
	}

	void cageGlfwInitializeFunc()
	{
		static CageGlfwInitializer *m = new CageGlfwInitializer(); // intentional leak
	}

	namespace
	{
		Mutex *windowsMutex()
		{
			static Holder<Mutex> *m = new Holder<Mutex>(newMutex()); // intentional leak
			return m->get();
		}

		struct Event
		{
			Event()
			{
				detail::memset(this, 0, sizeof(*this));
			}

			enum class TypeEnum
			{
				None,
				Close,
				Move,
				Resize,
				Show,
				Hide,
				FocusGain,
				FocusLose,
				KeyPress,
				KeyRelease,
				KeyRepeat,
				KeyChar,
				MousePress,
				MouseDouble,
				MouseRelease,
				MouseMove,
				MouseWheel,
#ifdef GCHL_WINDOWS_THREAD
				MouseOffsetAdd,
				//MouseOffsetReset,
#endif
			} type;

			ModifiersFlags modifiers;

			union
			{
				struct
				{
					ivec2 point;
					union
					{
						MouseButtonsFlags buttons;
						sint8 wheel;
					};
				} mouse;
				struct
				{
					uint32 key;
					uint32 scancode;
				} key;
				ivec2 point;
				uint32 codepoint;
			};
		};

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

		class WindowImpl : public Window
		{
		public:
			uint64 lastMouseButtonPressTimes[5] = { 0,0,0,0,0 }; // unused, left, right, unused, middle
			ConcurrentQueue<Event> eventsQueue;
			FlatSet<uint32> stateKeys, stateCodes;
			ivec2 stateMousePosition;
			MouseButtonsFlags stateButtons = MouseButtonsFlags::None;
			ModifiersFlags stateMods = ModifiersFlags::None;
			Window *shareContext = nullptr;
			GLFWwindow *window = nullptr;
			bool focus = true;

#ifdef GCHL_WINDOWS_THREAD
			Holder<Thread> windowThread;
			Holder<Semaphore> windowSemaphore;
			std::atomic<bool> stopping;
			ivec2 mouseOffsetApi;
			ConcurrentQueue<ivec2> mouseOffsetsThr;

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
						ScopeLock l(windowsMutex());
						glfwPollEvents();
					}

					ivec2 d;
					while (mouseOffsetsThr.tryPop(d))
					{
						double x, y;
						glfwGetCursorPos(window, &x, &y);
						x += d[0];
						y += d[1];
						glfwSetCursorPos(window, x, y);
						Event e;
						e.type = Event::TypeEnum::MouseOffsetAdd;
						e.modifiers = getKeyModifiers(window);
						e.point = -d;
						eventsQueue.push(e);
					}

					threadSleep(5000);
				}
				finalizeWindow();
			}
#endif

			explicit WindowImpl(Window *shareContext) : shareContext(shareContext)
			{
				cageGlfwInitializeFunc();

				ScopeLock l(windowsMutex());

#ifdef GCHL_WINDOWS_THREAD
				stopping = false;
				windowSemaphore = newSemaphore(0, 1);
				static uint32 threadIndex = 0;
				windowThread = newThread(Delegate<void()>().bind<WindowImpl, &WindowImpl::threadEntry>(this), stringizer() + "window " + (threadIndex++));
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
				ScopeLock lock(windowsMutex());
				glfwDestroyWindow(window);
				window = nullptr;
			}

			void initializeEvents();

			bool determineMouseDoubleClick(const Event &e)
			{
				if (e.type != Event::TypeEnum::MousePress)
					return false;
				CAGE_ASSERT((uint32)e.mouse.buttons < sizeof(lastMouseButtonPressTimes) / sizeof(lastMouseButtonPressTimes[0]));
				uint64 current = applicationTime();
				uint64 &last = lastMouseButtonPressTimes[(uint32)e.mouse.buttons];
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

			ivec2 offsetMousePositionApi(ivec2 p)
			{
				stateMousePosition = p;
#ifdef GCHL_WINDOWS_THREAD
				stateMousePosition += mouseOffsetApi;
#endif
				return stateMousePosition;
			}
		};

		void windowCloseEvent(GLFWwindow *w)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			e.type = Event::TypeEnum::Close;
			e.modifiers = getKeyModifiers(w);
			impl->eventsQueue.push(e);
		}

		void windowKeyCallback(GLFWwindow *w, int key, int scancode, int action, int mods)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			switch (action)
			{
			case GLFW_PRESS:
				e.type = Event::TypeEnum::KeyPress;
				break;
			case GLFW_REPEAT:
				e.type = Event::TypeEnum::KeyRepeat;
				break;
			case GLFW_RELEASE:
				e.type = Event::TypeEnum::KeyRelease;
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid key action");
			}
			e.modifiers = getKeyModifiers(mods);
			e.key.key = key;
			e.key.scancode = scancode;
			impl->eventsQueue.push(e);
		}

		void windowCharCallback(GLFWwindow *w, unsigned int codepoint)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			e.type = Event::TypeEnum::KeyChar;
			e.modifiers = getKeyModifiers(w);
			e.codepoint = codepoint;
			impl->eventsQueue.push(e);
		}

		void windowMouseMove(GLFWwindow *w, double xpos, double ypos)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			e.type = Event::TypeEnum::MouseMove;
			e.modifiers = getKeyModifiers(w);
			e.mouse.point[0] = numeric_cast<sint32>(floor(xpos));
			e.mouse.point[1] = numeric_cast<sint32>(floor(ypos));
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
				e.mouse.buttons |= MouseButtonsFlags::Left;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT))
				e.mouse.buttons |= MouseButtonsFlags::Right;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE))
				e.mouse.buttons |= MouseButtonsFlags::Middle;
			impl->eventsQueue.push(e);
		}

		void windowMouseButtonCallback(GLFWwindow *w, int button, int action, int mods)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			switch (action)
			{
			case GLFW_PRESS:
				e.type = Event::TypeEnum::MousePress;
				break;
			case GLFW_RELEASE:
				e.type = Event::TypeEnum::MouseRelease;
				break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid mouse action");
			}
			e.modifiers = getKeyModifiers(mods);
			switch (button)
			{
			case GLFW_MOUSE_BUTTON_LEFT:
				e.mouse.buttons = MouseButtonsFlags::Left;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				e.mouse.buttons = MouseButtonsFlags::Right;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				e.mouse.buttons = MouseButtonsFlags::Middle;
				break;
			default:
				return; // ignore other keys
			}
			double xpos, ypos;
			glfwGetCursorPos(w, &xpos, &ypos);
			e.mouse.point[0] = numeric_cast<sint32>(floor(xpos));
			e.mouse.point[1] = numeric_cast<sint32>(floor(ypos));
			bool doubleClick = impl->determineMouseDoubleClick(e);
			impl->eventsQueue.push(e);
			if (doubleClick)
			{
				e.type = Event::TypeEnum::MouseDouble;
				impl->eventsQueue.push(e);
			}
		}

		void windowMouseWheel(GLFWwindow *w, double, double yoffset)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			e.type = Event::TypeEnum::MouseWheel;
			e.modifiers = getKeyModifiers(w);
			double xpos, ypos;
			glfwGetCursorPos(w, &xpos, &ypos);
			e.mouse.point[0] = numeric_cast<sint32>(floor(xpos));
			e.mouse.point[1] = numeric_cast<sint32>(floor(ypos));
			e.mouse.wheel = numeric_cast<sint8>(yoffset);
			impl->eventsQueue.push(e);
		}

		void windowResizeCallback(GLFWwindow *w, int width, int height)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			e.type = Event::TypeEnum::Resize;
			e.modifiers = getKeyModifiers(w);
			e.point[0] = width;
			e.point[1] = height;
			impl->eventsQueue.push(e);
		}

		void windowMoveCallback(GLFWwindow *w, int xpos, int ypos)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			e.type = Event::TypeEnum::Move;
			e.modifiers = getKeyModifiers(w);
			e.point[0] = xpos;
			e.point[1] = ypos;
			impl->eventsQueue.push(e);
		}

		void windowIconifiedCallback(GLFWwindow *w, int iconified)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			if (iconified)
				e.type = Event::TypeEnum::Hide;
			else
				e.type = Event::TypeEnum::Show;
			e.modifiers = getKeyModifiers(w);
			impl->eventsQueue.push(e);
		}

		void windowMaximizedCallback(GLFWwindow *w, int maximized)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			e.type = Event::TypeEnum::Show; // window is visible when both maximized or restored
			e.modifiers = getKeyModifiers(w);
			impl->eventsQueue.push(e);
		}

		void windowFocusCallback(GLFWwindow *w, int focused)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			Event e;
			if (focused)
				e.type = Event::TypeEnum::FocusGain;
			else
				e.type = Event::TypeEnum::FocusLose;
			e.modifiers = getKeyModifiers(w);
			impl->eventsQueue.push(e);
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

	void Window::title(const string &value)
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

	void Window::setFullscreen(const ivec2 &resolution, uint32 frequency, const string &deviceId)
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

	ivec2 Window::mousePosition() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return impl->stateMousePosition;
	}

	void Window::mousePosition(const ivec2 &p)
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifdef GCHL_WINDOWS_THREAD
		ivec2 d = p - impl->stateMousePosition;
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

	bool Window::keyboardScanCode(uint32 code) const
	{
		WindowImpl *impl = (WindowImpl *)this;
		return impl->stateCodes.count(code);
	}

	void Window::processEvents()
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifndef GCHL_WINDOWS_THREAD
		{
			ScopeLock l(windowsMutex());
			glfwPollEvents();
		}
#endif
		Event e;
		while (impl->eventsQueue.tryPop(e))
		{
			impl->stateMods = e.modifiers;
			switch (e.type)
			{
			case Event::TypeEnum::Close:
				impl->events.windowClose.dispatch();
				break;
			case Event::TypeEnum::KeyPress:
				impl->stateKeys.insert(e.key.key);
				impl->stateCodes.insert(e.key.scancode);
				impl->events.keyPress.dispatch(e.key.key, e.key.scancode, e.modifiers);
				impl->events.keyRepeat.dispatch(e.key.key, e.key.scancode, e.modifiers);
				break;
			case Event::TypeEnum::KeyRepeat:
				impl->events.keyRepeat.dispatch(e.key.key, e.key.scancode, e.modifiers);
				break;
			case Event::TypeEnum::KeyRelease:
				impl->events.keyRelease.dispatch(e.key.key, e.key.scancode, e.modifiers);
				impl->stateKeys.erase(e.key.key);
				impl->stateCodes.erase(e.key.scancode);
				break;
			case Event::TypeEnum::KeyChar:
				impl->events.keyChar.dispatch(e.codepoint);
				break;
			case Event::TypeEnum::MouseMove:
				impl->events.mouseMove.dispatch(e.mouse.buttons, e.modifiers, impl->offsetMousePositionApi(e.mouse.point));
				break;
			case Event::TypeEnum::MousePress:
				impl->stateButtons |= e.mouse.buttons;
				impl->events.mousePress.dispatch(e.mouse.buttons, e.modifiers, impl->offsetMousePositionApi(e.mouse.point));
				break;
			case Event::TypeEnum::MouseDouble:
				impl->events.mouseDouble.dispatch(e.mouse.buttons, e.modifiers, impl->offsetMousePositionApi(e.mouse.point));
				break;
			case Event::TypeEnum::MouseRelease:
				impl->events.mouseRelease.dispatch(e.mouse.buttons, e.modifiers, impl->offsetMousePositionApi(e.mouse.point));
				impl->stateButtons &= ~e.mouse.buttons;
				break;
			case Event::TypeEnum::MouseWheel:
				impl->events.mouseWheel.dispatch(e.mouse.wheel, e.modifiers, impl->offsetMousePositionApi(e.mouse.point));
				break;
			case Event::TypeEnum::Resize:
				impl->events.windowResize.dispatch(e.point);
				break;
			case Event::TypeEnum::Move:
				impl->events.windowMove.dispatch(e.point);
				break;
			case Event::TypeEnum::Show:
				impl->events.windowShow.dispatch();
				break;
			case Event::TypeEnum::Hide:
				impl->events.windowHide.dispatch();
				break;
			case Event::TypeEnum::FocusGain:
				impl->focus = true;
				impl->events.focusGain.dispatch();
				break;
			case Event::TypeEnum::FocusLose:
				impl->focus = false;
				impl->events.focusLose.dispatch();
				break;
#ifdef GCHL_WINDOWS_THREAD
			case Event::TypeEnum::MouseOffsetAdd:
				impl->mouseOffsetApi += e.point;
				break;
#endif
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid window event type");
			}
		}
	}

	ivec2 Window::resolution() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		int w = 0, h = 0;
		glfwGetFramebufferSize(impl->window, &w, &h);
		return ivec2(w, h);
	}

	real Window::contentScaling() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		float x = 1, y = 1;
		glfwGetWindowContentScale(impl->window, &x, &y);
		return max(x, y);
	}

	string Window::screenId() const
	{
		WindowImpl *impl = (WindowImpl *)this;
		{
			auto m = glfwGetWindowMonitor(impl->window);
			if (m)
				return getMonitorId(m);
		}
		const vec2 center = vec2(windowedPosition() + windowedSize() / 2);
		int cnt = 0;
		GLFWmonitor **ms = glfwGetMonitors(&cnt);
		// todo first filter out monitors that do not overlap with the center of the window
		uint32 bestIndex = m;
		real bestDist = real::Infinity();
		for (uint32 i = 0; i < numeric_cast<uint32>(cnt); i++)
		{
			int x, y, w, h;
			glfwGetMonitorWorkarea(ms[i], &x, &y, &w, &h);
			const real dist = distance(center, vec2(x, y) + vec2(w, h) / 2);
			if (dist < bestDist)
			{
				bestDist = dist;
				bestIndex = i;
			}
		}
		CAGE_ASSERT(bestIndex != m);
		return getMonitorId(ms[bestIndex]);
	}

	ivec2 Window::windowedSize() const
	{
		WindowImpl *impl = (WindowImpl*)this;
		int w = 0, h = 0;
		glfwGetWindowSize(impl->window, &w, &h);
		return ivec2(w, h);
	}

	void Window::windowedSize(const ivec2 &tmp)
	{
		WindowImpl *impl = (WindowImpl*)this;
		glfwSetWindowSize(impl->window, tmp[0], tmp[1]);
	}

	ivec2 Window::windowedPosition() const
	{
		WindowImpl *impl = (WindowImpl*)this;
		int x = 0, y = 0;
		glfwGetWindowPos(impl->window, &x, &y);
		return ivec2(x, y);
	}

	void Window::windowedPosition(const ivec2 &tmp)
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
		return systemArena().createImpl<Window, WindowImpl>(shareContext);
	}

	void WindowEventListeners::attachAll(Window *window, sint32 order)
	{
#define GCHL_GENERATE(T) if(window) T.attach(window->events.T, order); else T.detach();
		GCHL_GENERATE(windowClose);
		GCHL_GENERATE(windowShow);
		GCHL_GENERATE(windowHide);
		GCHL_GENERATE(windowMove);
		GCHL_GENERATE(windowResize);
		GCHL_GENERATE(mouseMove);
		GCHL_GENERATE(mousePress);
		GCHL_GENERATE(mouseDouble);
		GCHL_GENERATE(mouseRelease);
		GCHL_GENERATE(mouseWheel);
		GCHL_GENERATE(focusGain);
		GCHL_GENERATE(focusLose);
		GCHL_GENERATE(keyPress);
		GCHL_GENERATE(keyRelease);
		GCHL_GENERATE(keyRepeat);
		GCHL_GENERATE(keyChar);
#undef GCHL_GENERATE
	}
}
