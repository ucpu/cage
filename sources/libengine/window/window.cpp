#include <atomic>
#include <vector>

#include "private.h"

#include <cage-core/concurrent.h>
#include <cage-core/concurrentQueue.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/flatSet.h>
#include <cage-engine/window.h>

#ifdef CAGE_SYSTEM_WINDOWS
	#define GCHL_WINDOWS_THREAD
#endif // CAGE_SYSTEM_WINDOWS

namespace cage
{
	namespace detail
	{
		void initializeOpengl();
		void configureOpengl(Window *w);
	}

	namespace
	{
		const ConfigBool confDebugContext("cage/graphics/debugContext", CAGE_DEBUG_BOOL);
		const ConfigBool confNoErrorContext("cage/graphics/noErrorContext", false);

		ModifiersFlags getKeyModifiers(int mods)
		{
			return ModifiersFlags::None | ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT ? ModifiersFlags::Shift : ModifiersFlags::None) | ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL ? ModifiersFlags::Ctrl : ModifiersFlags::None) | ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT ? ModifiersFlags::Alt : ModifiersFlags::None) | ((mods & GLFW_MOD_SUPER) == GLFW_MOD_SUPER ? ModifiersFlags::Super : ModifiersFlags::None);
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
			Vec2 off;
		};

		class WindowImpl : public Window
		{
		public:
			Vec2 lastMouseButtonPressPositions[5] = {};
			uint64 lastMouseButtonPressTimes[5] = {}; // unused, left, right, unused, middle

			ConcurrentQueue<GenericInput> eventsQueue;
			FlatSet<uint32> stateKeys;
			MouseButtonsFlags stateButtons = MouseButtonsFlags::None;
			ModifiersFlags stateMods = ModifiersFlags::None;
			Vec2 stateMousePosition;
			Vec2 lastRelativePosition;
			Window *const shareContext = nullptr;
			GLFWwindow *window = nullptr;
			const int vsync = -1;
			bool focus = true;
			bool mouseIntendVisible = true;
			bool mouseIntendRelative = false;

#ifdef CAGE_DEBUG
			uint64 currentThreadIdGlBound = 0;
#endif

#ifdef GCHL_WINDOWS_THREAD
			Holder<Thread> windowThread;
			Holder<Semaphore> windowSemaphore;
			std::atomic<bool> stopping = false;

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
					updateMouseMode();
					{
						ScopeLock l(cageGlfwMutex());
						glfwPollEvents();
					}
					threadSleep(5000);
				}
				finalizeWindow();
			}
#endif

			explicit WindowImpl(const WindowCreateConfig &config) : shareContext(config.shareContext), vsync(config.vsync)
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
				detail::initializeOpengl();
				detail::configureOpengl(this);
				if (vsync >= 0)
					glfwSwapInterval(vsync > 0 ? 1 : 0);
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
				//glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE); // GLFW_FALSE is useful for multiple simultaneous fullscreen windows but breaks alt-tabbing out of the application
				glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, confDebugContext ? GLFW_TRUE : GLFW_FALSE);
				if (confDebugContext)
					CAGE_LOG(SeverityEnum::Info, "graphics", "creating DEBUG opengl context");
				const bool noErr = confNoErrorContext && !confDebugContext;
				glfwWindowHint(GLFW_CONTEXT_NO_ERROR, noErr ? GLFW_TRUE : GLFW_FALSE);
				if (noErr)
					CAGE_LOG(SeverityEnum::Info, "graphics", "creating NO-ERROR opengl context");
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				glfwWindowHint(GLFW_ALPHA_BITS, 0); // save some gpu memory
				glfwWindowHint(GLFW_DEPTH_BITS, 0);
				glfwWindowHint(GLFW_STENCIL_BITS, 0);
				const String name = pathExtractFilename(detail::executableFullPathNoExe());
				window = glfwCreateWindow(1, 1, name.c_str(), NULL, shareContext ? ((WindowImpl *)shareContext)->window : NULL);
				if (!window)
					CAGE_THROW_ERROR(Exception, "failed to create window");
				glfwSetWindowUserPointer(window, this);
				glfwSetWindowSizeLimits(window, 800, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
				//if (glfwRawMouseMotionSupported())
				//	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
				initializeEvents();
			}

			void finalizeWindow()
			{
				ScopeLock lock(cageGlfwMutex());
				glfwDestroyWindow(window);
				window = nullptr;
			}

			void initializeEvents();

			void updateMouseMode()
			{
				int intent = GLFW_CURSOR_NORMAL;
				if (!mouseIntendVisible)
					intent = GLFW_CURSOR_HIDDEN;
				if (mouseIntendRelative)
					intent = GLFW_CURSOR_DISABLED;
				glfwSetInputMode(window, GLFW_CURSOR, intent);
			}

			template<class T>
			void updateMouseStateAndDispatch(GenericInput in)
			{
				T i = in.get<T>();
				stateMods = i.mods;
				if constexpr (std::is_same_v<T, input::MousePress>)
					stateButtons |= i.buttons;
				if constexpr (std::is_same_v<T, input::MouseRelease>)
					stateButtons &= ~i.buttons;
				if constexpr (std::is_same_v<T, input::MouseMove>)
				{
					CAGE_ASSERT(!i.relative);
				}
				if constexpr (std::is_same_v<T, input::MouseRelativeMove>)
				{
					CAGE_ASSERT(i.relative);
					const Vec2 n = i.position;
					if (valid(lastRelativePosition))
						i.position = n - lastRelativePosition;
					else
						i.position = Vec2();
					lastRelativePosition = n;
					stateMousePosition = Vec2(resolution()) * 0.5;
				}
				else if (i.relative)
					stateMousePosition = i.position = Vec2(resolution()) * 0.5;
				else
					stateMousePosition = i.position;
				events.dispatch(i);
			}

			bool getRelative() const { return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED; }

			Vec2 getPosition()
			{
				Vec2 pos;
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				pos[0] = xpos;
				pos[1] = ypos;
				return pos;
			}

			bool determineMouseDoubleClick(MouseButtonsFlags buttons, Vec2 cp)
			{
				CAGE_ASSERT((uint32)buttons < array_size(lastMouseButtonPressTimes));
				const uint64 ct = applicationTime();
				uint64 &lt = lastMouseButtonPressTimes[(uint32)buttons];
				Vec2 &lp = lastMouseButtonPressPositions[(uint32)buttons];
				if (ct - lt < 300000 && distance(cp, lp) < 5)
				{
					lt = 0;
					return true;
				}
				else
				{
					lt = ct;
					lp = cp;
					return false;
				}
			}
		};

		void windowCloseCallback(GLFWwindow *w)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			impl->eventsQueue.push(input::WindowClose{ impl });
		}

		void windowKeyCallback(GLFWwindow *w, int key, int, int action, int mods)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			ModifiersFlags ms = getKeyModifiers(mods);

#ifndef CAGE_SYSTEM_WINDOWS
			// https://github.com/glfw/glfw/issues/1630
			ModifiersFlags md = ModifiersFlags::None;
			switch (key)
			{
				case GLFW_KEY_LEFT_SHIFT:
				case GLFW_KEY_RIGHT_SHIFT:
					md = ModifiersFlags::Shift;
					break;
				case GLFW_KEY_LEFT_CONTROL:
				case GLFW_KEY_RIGHT_CONTROL:
					md = ModifiersFlags::Ctrl;
					break;
				case GLFW_KEY_LEFT_ALT:
				case GLFW_KEY_RIGHT_ALT:
					md = ModifiersFlags::Alt;
					break;
				case GLFW_KEY_LEFT_SUPER:
				case GLFW_KEY_RIGHT_SUPER:
					md = ModifiersFlags::Super;
					break;
			}
			switch (action)
			{
				case GLFW_PRESS:
				case GLFW_REPEAT:
					ms |= md;
					break;
				case GLFW_RELEASE:
					ms &= ~md;
					break;
			}
#endif // CAGE_SYSTEM_WINDOWS

			switch (action)
			{
				case GLFW_PRESS:
					impl->eventsQueue.push(input::KeyPress{ impl, (uint32)key, ms });
					break;
				case GLFW_REPEAT:
					impl->eventsQueue.push(input::KeyRepeat{ impl, (uint32)key, ms });
					break;
				case GLFW_RELEASE:
					impl->eventsQueue.push(input::KeyRelease{ impl, (uint32)key, ms });
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid key action");
			}
		}

		void windowCharCallback(GLFWwindow *w, unsigned int codepoint)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			const ModifiersFlags ms = getKeyModifiers(w);
			impl->eventsQueue.push(input::Character{ impl, (uint32)codepoint, ms });
		}

		void windowMouseMoveCallback(GLFWwindow *w, double xpos, double ypos)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			input::privat::BaseMouse e;
			e.window = impl;
			e.mods = getKeyModifiers(w);
			e.position[0] = xpos;
			e.position[1] = ypos;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
				e.buttons |= MouseButtonsFlags::Left;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT))
				e.buttons |= MouseButtonsFlags::Right;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE))
				e.buttons |= MouseButtonsFlags::Middle;
			e.relative = impl->getRelative();
			static_assert(sizeof(input::MouseRelativeMove) == sizeof(input::privat::BaseMouse));
			static_assert(sizeof(input::MouseMove) == sizeof(input::privat::BaseMouse));
			if (e.relative)
				impl->eventsQueue.push(*(input::MouseRelativeMove *)(&e));
			else
				impl->eventsQueue.push(*(input::MouseMove *)(&e));
		}

		void windowMouseButtonCallback(GLFWwindow *w, int button, int action, int mods)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			const ModifiersFlags ms = getKeyModifiers(mods);
			MouseButtonsFlags bf = MouseButtonsFlags::None;
			switch (button)
			{
				case GLFW_MOUSE_BUTTON_LEFT:
					bf = MouseButtonsFlags::Left;
					break;
				case GLFW_MOUSE_BUTTON_RIGHT:
					bf = MouseButtonsFlags::Right;
					break;
				case GLFW_MOUSE_BUTTON_MIDDLE:
					bf = MouseButtonsFlags::Middle;
					break;
				default:
					return; // ignore other keys
			}
			const Vec2 pos = impl->getPosition();
			const bool relative = impl->getRelative();

			switch (action)
			{
				case GLFW_PRESS:
					impl->eventsQueue.push(input::MousePress{ impl, pos, bf, ms, relative });
					break;
				case GLFW_RELEASE:
					impl->eventsQueue.push(input::MouseRelease{ impl, pos, bf, ms, relative });
					break;
				default:
					CAGE_THROW_CRITICAL(Exception, "invalid mouse action");
			}

			if (action == GLFW_PRESS && impl->determineMouseDoubleClick(bf, pos))
				impl->eventsQueue.push(input::MouseDoublePress{ impl, pos, bf, ms, relative });
		}

		void windowMouseWheelCallback(GLFWwindow *w, double, double yoffset)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			const ModifiersFlags ms = getKeyModifiers(w);
			impl->eventsQueue.push(input::MouseWheel{ impl, impl->getPosition(), Real(yoffset), ms, impl->getRelative() });
		}

		void windowResizeCallback(GLFWwindow *w, int width, int height)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			impl->eventsQueue.push(input::WindowResize{ impl, Vec2i(width, height) });
		}

		void windowMoveCallback(GLFWwindow *w, int xpos, int ypos)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			impl->eventsQueue.push(input::WindowMove{ impl, Vec2i(xpos, ypos) });
		}

		void windowIconifiedCallback(GLFWwindow *w, int iconified)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			if (iconified)
				impl->eventsQueue.push(input::WindowHide{ impl });
			else
				impl->eventsQueue.push(input::WindowShow{ impl });
		}

		void windowMaximizedCallback(GLFWwindow *w, int maximized)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			impl->eventsQueue.push(input::WindowShow{ impl }); // window is visible when both maximized or restored
		}

		void windowFocusCallback(GLFWwindow *w, int focused)
		{
			WindowImpl *impl = (WindowImpl *)glfwGetWindowUserPointer(w);
			if (focused)
				impl->eventsQueue.push(input::WindowFocusGain{ impl });
			else
				impl->eventsQueue.push(input::WindowFocusLose{ impl });
		}

		void WindowImpl::initializeEvents()
		{
			glfwSetWindowCloseCallback(window, windowCloseCallback);
			glfwSetKeyCallback(window, windowKeyCallback);
			glfwSetCharCallback(window, windowCharCallback);
			glfwSetCursorPosCallback(window, windowMouseMoveCallback);
			glfwSetMouseButtonCallback(window, windowMouseButtonCallback);
			glfwSetScrollCallback(window, windowMouseWheelCallback);
			glfwSetWindowSizeCallback(window, windowResizeCallback);
			glfwSetWindowPosCallback(window, windowMoveCallback);
			glfwSetWindowIconifyCallback(window, windowIconifiedCallback);
			glfwSetWindowMaximizeCallback(window, windowMaximizedCallback);
			glfwSetWindowFocusCallback(window, windowFocusCallback);
		}
	}

	String Window::title() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		if (const char *tmp = glfwGetWindowTitle(impl->window))
		{
			const auto len = std::strlen(tmp);
			if (len < String::MaxLength)
				return String(PointerRange(tmp, tmp + len));
		}
		return {};
	}

	void Window::title(const String &value)
	{
		WindowImpl *impl = (WindowImpl *)this;
		glfwSetWindowTitle(impl->window, value.c_str());
	}

	bool Window::isFocused() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
#ifdef GCHL_WINDOWS_THREAD
		return impl->focus;
#else
		return glfwGetWindowAttrib(impl->window, GLFW_FOCUSED);
#endif
	}

	bool Window::isFullscreen() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return !!glfwGetWindowMonitor(impl->window);
	}

	bool Window::isMaximized() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return !!glfwGetWindowAttrib(impl->window, GLFW_MAXIMIZED);
	}

	bool Window::isWindowed() const
	{
		return !isHidden() && !isMinimized() && !isMaximized() && !isFullscreen();
	}

	bool Window::isMinimized() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return !!glfwGetWindowAttrib(impl->window, GLFW_ICONIFIED);
	}

	bool Window::isHidden() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return !glfwGetWindowAttrib(impl->window, GLFW_VISIBLE);
	}

	bool Window::isVisible() const
	{
		return !isHidden() && !isMinimized();
	}

	namespace
	{
		void normalizeWindow(WindowImpl *impl, WindowFlags flags, bool fullscreen = false)
		{
			if (impl->isFullscreen())
				glfwSetWindowMonitor(impl->window, nullptr, 100, 100, 800, 600, 0);
			if (impl->isHidden())
				glfwShowWindow(impl->window);
			if (impl->isMinimized() || impl->isMaximized())
				glfwRestoreWindow(impl->window);

				// linux: the window minimizes immediately after fullscreen if these attributes are changed
#ifdef CAGE_SYSTEM_WINDOWS
			fullscreen = false;
#endif
			if (!fullscreen)
			{
				glfwSetWindowAttrib(impl->window, GLFW_DECORATED, any(flags & WindowFlags::Border));
				glfwSetWindowAttrib(impl->window, GLFW_RESIZABLE, any(flags & WindowFlags::Resizeable));
			}
		}
	}

	void Window::setFullscreen(Vec2i resolution, uint32 frequency, const String &deviceId)
	{
		WindowImpl *impl = (WindowImpl *)this;
		normalizeWindow(impl, WindowFlags::None, true);
		GLFWmonitor *m = getMonitorById(deviceId);
		CAGE_ASSERT(m);
		const GLFWvidmode *v = glfwGetVideoMode(m);
		CAGE_ASSERT(v);
		glfwSetWindowMonitor(impl->window, m, 0, 0, resolution[0] > 0 ? resolution[0] : v->width, resolution[1] > 0 ? resolution[1] : v->height, frequency > 0 ? frequency : v->refreshRate);
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
		const WindowImpl *impl = (const WindowImpl *)this;
		return impl->mouseIntendVisible && !impl->mouseIntendRelative;
	}

	void Window::mouseVisible(bool visible)
	{
		WindowImpl *impl = (WindowImpl *)this;
		impl->mouseIntendVisible = visible;
	}

	bool Window::mouseRelativeMovement() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return impl->mouseIntendRelative;
	}

	void Window::mouseRelativeMovement(bool relative)
	{
		WindowImpl *impl = (WindowImpl *)this;
		impl->mouseIntendRelative = relative;
		impl->lastRelativePosition = Vec2::Nan();
	}

	Vec2 Window::mousePosition() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return impl->stateMousePosition;
	}

	void Window::mousePosition(Vec2 p)
	{
		WindowImpl *impl = (WindowImpl *)this;
		CAGE_ASSERT(!impl->mouseIntendRelative);
		glfwSetCursorPos(impl->window, p[0].value, p[1].value);
	}

	MouseButtonsFlags Window::mouseButtons() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return impl->stateButtons;
	}

	ModifiersFlags Window::keyboardModifiers() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return impl->stateMods;
	}

	bool Window::keyboardKey(uint32 key) const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		return impl->stateKeys.count(key);
	}

	void Window::processEvents()
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifndef GCHL_WINDOWS_THREAD
		impl->updateMouseMode();
		{
			ScopeLock l(cageGlfwMutex());
			glfwPollEvents();
		}
#endif
		GenericInput e;
		while (impl->eventsQueue.tryPop(e))
		{
			switch (e.typeHash())
			{
				case detail::typeHash<input::MouseMove>():
					impl->updateMouseStateAndDispatch<input::MouseMove>(e);
					break;
				case detail::typeHash<input::MouseRelativeMove>():
					impl->updateMouseStateAndDispatch<input::MouseRelativeMove>(e);
					break;
				case detail::typeHash<input::MousePress>():
					impl->updateMouseStateAndDispatch<input::MousePress>(e);
					break;
				case detail::typeHash<input::MouseDoublePress>():
					impl->updateMouseStateAndDispatch<input::MouseDoublePress>(e);
					break;
				case detail::typeHash<input::MouseRelease>():
					impl->updateMouseStateAndDispatch<input::MouseRelease>(e);
					break;
				case detail::typeHash<input::MouseWheel>():
					impl->updateMouseStateAndDispatch<input::MouseWheel>(e);
					break;
				case detail::typeHash<input::KeyPress>():
				{
					impl->stateKeys.insert(e.get<input::KeyPress>().key);
					impl->stateMods = e.get<input::KeyPress>().mods;
					events.dispatch(e);
					const input::KeyPress p = e.get<input::KeyPress>();
					events.dispatch(input::KeyRepeat{ p.window, p.key, p.mods });
					break;
				}
				case detail::typeHash<input::KeyRelease>():
					impl->stateKeys.erase(e.get<input::KeyRelease>().key);
					impl->stateMods = e.get<input::KeyRelease>().mods;
					events.dispatch(e);
					break;
				case detail::typeHash<input::KeyRepeat>():
					impl->stateKeys.insert(e.get<input::KeyRepeat>().key);
					impl->stateMods = e.get<input::KeyRepeat>().mods;
					events.dispatch(e);
					break;
				case detail::typeHash<input::Character>():
					impl->stateMods = e.get<input::Character>().mods;
					events.dispatch(e);
					break;
				case detail::typeHash<input::WindowFocusGain>():
					impl->focus = true;
					events.dispatch(e);
					break;
				case detail::typeHash<input::WindowFocusLose>():
					impl->focus = false;
					events.dispatch(e);
					break;
				default:
					events.dispatch(e);
					break;
			}
		}
	}

	Vec2i Window::resolution() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		int w = 0, h = 0;
		glfwGetFramebufferSize(impl->window, &w, &h);
		return Vec2i(w, h);
	}

	Real Window::contentScaling() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		float x = 1, y = 1;
		glfwGetWindowContentScale(impl->window, &x, &y);
		return max(x, y);
	}

	String Window::screenId() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
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
		const WindowImpl *impl = (const WindowImpl *)this;
		int w = 0, h = 0;
		glfwGetWindowSize(impl->window, &w, &h);
		return Vec2i(w, h);
	}

	void Window::windowedSize(Vec2i tmp)
	{
		WindowImpl *impl = (WindowImpl *)this;
		glfwSetWindowSize(impl->window, tmp[0], tmp[1]);
	}

	Vec2i Window::windowedPosition() const
	{
		const WindowImpl *impl = (const WindowImpl *)this;
		int x = 0, y = 0;
		glfwGetWindowPos(impl->window, &x, &y);
		return Vec2i(x, y);
	}

	void Window::windowedPosition(Vec2i tmp)
	{
		WindowImpl *impl = (WindowImpl *)this;
		glfwSetWindowPos(impl->window, tmp[0], tmp[1]);
	}

#ifdef CAGE_DEBUG
	namespace
	{
		Holder<Mutex> mutex = newMutex();
	}
#endif

	void Window::makeCurrent()
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifdef CAGE_DEBUG
		{
			ScopeLock _(mutex);
			CAGE_ASSERT(impl->currentThreadIdGlBound == 0);
			impl->currentThreadIdGlBound = currentThreadId();
		}
#endif
		glfwMakeContextCurrent(impl->window);
	}

	void Window::makeNotCurrent()
	{
#ifdef CAGE_DEBUG
		{
			WindowImpl *impl = (WindowImpl *)this;
			ScopeLock _(mutex);
			CAGE_ASSERT(impl->currentThreadIdGlBound == currentThreadId());
			impl->currentThreadIdGlBound = 0;
		}
#endif
		glfwMakeContextCurrent(nullptr);
	}

	void Window::swapBuffers()
	{
		WindowImpl *impl = (WindowImpl *)this;
#ifdef CAGE_DEBUG
		{
			ScopeLock _(mutex);
			CAGE_ASSERT(impl->currentThreadIdGlBound == currentThreadId());
		}
#endif
		glfwSwapBuffers(impl->window);
	}

	Holder<Window> newWindow(const WindowCreateConfig &config)
	{
		cageGlfwInitializeFunc();
		return systemMemory().createImpl<Window, WindowImpl>(config);
	}

	GLFWwindow *getGlfwWindow(Window *w)
	{
		CAGE_ASSERT(w);
		WindowImpl *impl = (WindowImpl *)w;
		return impl->window;
	}
}
