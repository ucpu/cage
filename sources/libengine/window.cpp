#include <atomic>
#include <vector>
#include <set>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/config.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-engine/core.h>
#include <cage-engine/window.h>
#include <cage-engine/graphics.h>
#include <cage-engine/opengl.h>
#include "graphics/private.h"
#include <GLFW/glfw3.h>

#ifdef CAGE_SYSTEM_WINDOWS
#define GCHL_WINDOWS_THREAD
#endif // CAGE_SYSTEM_WINDOWS

namespace cage
{
	namespace
	{
		configBool confDebugContext("cage.graphics.debugContext",
#ifdef CAGE_DEBUG
			true
#else
			false
#endif // CAGE_DEBUG
			);
		configBool confNoErrorContext("cage.graphics.noErrorContext", false);

		void handleGlfwError(int, const char *message)
		{
			CAGE_LOG(severityEnum::Error, "glfw", message);
		}

		class cageGlfwInitializerClass
		{
		public:
			cageGlfwInitializerClass()
			{
				glfwSetErrorCallback(&handleGlfwError);
				if (!glfwInit())
					CAGE_THROW_CRITICAL(exception, "failed to initialize glfw");
			}

			~cageGlfwInitializerClass()
			{
				glfwTerminate();
			}
		};
	}

	void cageGlfwInitializeFunc()
	{
		static cageGlfwInitializerClass *m = new cageGlfwInitializerClass(); // intentional leak
		(void)m;
	}

	namespace
	{
		syncMutex *windowsMutex()
		{
			static holder<syncMutex> *m = new holder<syncMutex>(newSyncMutex()); // intentional leak
			return m->get();
		}

		struct eventStruct
		{
			eventStruct()
			{
				detail::memset(this, 0, sizeof(*this));
			}

			enum class eventType
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
				DropRelativeMouseOffset,
#endif
			} type;

			modifiersFlags modifiers;

			union
			{
				struct
				{
					sint32 x, y;
					union
					{
						mouseButtonsFlags buttons;
						sint8 wheel;
					};
				} mouse;
				struct
				{
					uint32 key;
					uint32 scancode;
				} key;
				struct
				{
					sint32 x;
					sint32 y;
				} point;
				uint32 codepoint;
			};
		};

		modifiersFlags getKeyModifiers(int mods)
		{
			return modifiersFlags::None
				| ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT ? modifiersFlags::Shift : modifiersFlags::None)
				| ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL ? modifiersFlags::Ctrl : modifiersFlags::None)
				| ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT ? modifiersFlags::Alt : modifiersFlags::None)
				| ((mods & GLFW_MOD_SUPER) == GLFW_MOD_SUPER ? modifiersFlags::Super : modifiersFlags::None);
		}

		modifiersFlags getKeyModifiers(GLFWwindow *w)
		{
			modifiersFlags r = modifiersFlags::None;
			if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
				r |= modifiersFlags::Shift;
			if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
				r |= modifiersFlags::Ctrl;
			if (glfwGetKey(w, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
				r |= modifiersFlags::Alt;
			if (glfwGetKey(w, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
				r |= modifiersFlags::Super;
			return r;
		}

		class windowImpl : public windowHandle
		{
		public:
			uint64 lastMouseButtonPressTimes[5]; // unused, left, right, unused, middle
			windowHandle *shareContext;
			holder<syncMutex> eventsMutex;
			std::vector<eventStruct> eventsQueueLocked;
			std::vector<eventStruct> eventsQueueNoLock;
			std::set<uint32> stateKeys, stateCodes;
			ivec2 currentMousePosition;
			modifiersFlags stateMods;
			mouseButtonsFlags stateButtons;
			GLFWwindow *window;
			bool focus;

#ifdef GCHL_WINDOWS_THREAD
			holder<threadHandle> windowThread;
			holder<syncSemaphore> windowSemaphore;
			std::atomic<bool> stopping;
			std::vector<ivec2> relativeMouseOffsets;
			std::vector<ivec2> absoluteMouseSets;

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
					try
					{
						{
							scopeLock<syncMutex> l(windowsMutex());
							glfwPollEvents();
						}
						
						{
							scopeLock<syncMutex> l(eventsMutex);
							for (const auto &p : absoluteMouseSets)
							{
								eventStruct e;
								e.type = eventStruct::eventType::DropRelativeMouseOffset;
								e.modifiers = getKeyModifiers(window);
								eventsQueueLocked.push_back(e);
								glfwSetCursorPos(window, p.x, p.y);
							}
							absoluteMouseSets.clear();
						}
					}
					catch (...)
					{
						CAGE_LOG(severityEnum::Warning, "window", "an exception occured in event processing");
					}
					threadSleep(2000);
				}
				finalizeWindow();
			}
#endif

			windowImpl(windowHandle *shareContext) : lastMouseButtonPressTimes{0,0,0,0,0}, shareContext(shareContext), eventsMutex(newSyncMutex()), stateMods(modifiersFlags::None), stateButtons(mouseButtonsFlags::None), window(nullptr), focus(true)
			{
				cageGlfwInitializeFunc();

				scopeLock<syncMutex> l(windowsMutex());

#ifdef GCHL_WINDOWS_THREAD
				stopping = false;
				windowSemaphore = newSyncSemaphore(0, 1);
				static uint32 threadIndex = 0;
				windowThread = newThread(delegate<void()>().bind<windowImpl, &windowImpl::threadEntry>(this), string() + "window " + (threadIndex++));
				windowSemaphore->lock();
				windowSemaphore.clear();
				if (stopping)
					CAGE_THROW_ERROR(exception, "failed to initialize window");
#else
				initializeWindow();
#endif

				// initialize opengl
				makeCurrent();
				if (!gladLoadGL())
					CAGE_THROW_ERROR(exception, "gladLoadGl");
				openglContextInitializeGeneral(this);
			}

			~windowImpl()
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
				glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
				glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, confDebugContext ? GLFW_TRUE : GLFW_FALSE);
				if (confDebugContext)
					CAGE_LOG(severityEnum::Info, "graphics", "creating DEBUG opengl context");
				bool noErr = confNoErrorContext && !confDebugContext;
				glfwWindowHint(GLFW_CONTEXT_NO_ERROR, noErr ? GLFW_TRUE : GLFW_FALSE);
				if (noErr)
					CAGE_LOG(severityEnum::Info, "graphics", "creating NO ERROR opengl context");
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				glfwWindowHint(GLFW_ALPHA_BITS, 0); // save some gpu memory
				glfwWindowHint(GLFW_DEPTH_BITS, 0);
				glfwWindowHint(GLFW_STENCIL_BITS, 0);
				window = glfwCreateWindow(1, 1, "Cage Window", NULL, shareContext ? ((windowImpl*)shareContext)->window : NULL);
				if (!window)
					CAGE_THROW_ERROR(exception, "failed to create window");
				glfwSetWindowUserPointer(window, this);
				glfwSetWindowSizeLimits(window, 160, 120, GLFW_DONT_CARE, GLFW_DONT_CARE);
				initializeEvents();
			}

			void finalizeWindow()
			{
				scopeLock<syncMutex> lock(windowsMutex());
				glfwDestroyWindow(window);
				window = nullptr;
			}

			void initializeEvents();

			bool determineMouseDoubleClick(const eventStruct &e)
			{
				if (e.type != eventStruct::eventType::MousePress)
					return false;
				CAGE_ASSERT((uint32)e.mouse.buttons < sizeof(lastMouseButtonPressTimes) / sizeof(lastMouseButtonPressTimes[0]));
				uint64 current = getApplicationTime();
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

			ivec2 processMousePositionEvent(ivec2 p)
			{
				currentMousePosition = p;
#ifdef GCHL_WINDOWS_THREAD
				for (const auto &r : relativeMouseOffsets)
				{
					currentMousePosition.x += r.x;
					currentMousePosition.y += r.y;
				}
#endif
				return currentMousePosition;
			}
		};

		void windowCloseEvent(GLFWwindow *w)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Close;
			e.modifiers = getKeyModifiers(w);
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowKeyCallback(GLFWwindow *w, int key, int scancode, int action, int mods)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			switch (action)
			{
			case GLFW_PRESS:
				e.type = eventStruct::eventType::KeyPress;
				break;
			case GLFW_REPEAT:
				e.type = eventStruct::eventType::KeyRepeat;
				break;
			case GLFW_RELEASE:
				e.type = eventStruct::eventType::KeyRelease;
				break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid key action");
			}
			e.modifiers = getKeyModifiers(mods);
			e.key.key = key;
			e.key.scancode = scancode;
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowCharCallback(GLFWwindow *w, unsigned int codepoint)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::KeyChar;
			e.modifiers = getKeyModifiers(w);
			e.codepoint = codepoint;
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowMouseMove(GLFWwindow *w, double xpos, double ypos)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::MouseMove;
			e.modifiers = getKeyModifiers(w);
			e.mouse.x = numeric_cast<sint32>(floor(xpos));
			e.mouse.y = numeric_cast<sint32>(floor(ypos));
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
				e.mouse.buttons |= mouseButtonsFlags::Left;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT))
				e.mouse.buttons |= mouseButtonsFlags::Right;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE))
				e.mouse.buttons |= mouseButtonsFlags::Middle;
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowMouseButtonCallback(GLFWwindow *w, int button, int action, int mods)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			switch (action)
			{
			case GLFW_PRESS:
				e.type = eventStruct::eventType::MousePress;
				break;
			case GLFW_RELEASE:
				e.type = eventStruct::eventType::MouseRelease;
				break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid mouse action");
			}
			e.modifiers = getKeyModifiers(mods);
			switch (button)
			{
			case GLFW_MOUSE_BUTTON_LEFT:
				e.mouse.buttons = mouseButtonsFlags::Left;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				e.mouse.buttons = mouseButtonsFlags::Right;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				e.mouse.buttons = mouseButtonsFlags::Middle;
				break;
			default:
				return; // ignore other keys
			}
			double xpos, ypos;
			glfwGetCursorPos(w, &xpos, &ypos);
			e.mouse.x = numeric_cast<sint32>(floor(xpos));
			e.mouse.y = numeric_cast<sint32>(floor(ypos));
			bool doubleClick = impl->determineMouseDoubleClick(e);
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
			if (doubleClick)
			{
				e.type = eventStruct::eventType::MouseDouble;
				impl->eventsQueueLocked.push_back(e);
			}
		}

		void windowMouseWheel(GLFWwindow *w, double, double yoffset)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::MouseWheel;
			e.modifiers = getKeyModifiers(w);
			double xpos, ypos;
			glfwGetCursorPos(w, &xpos, &ypos);
			e.mouse.x = numeric_cast<sint32>(floor(xpos));
			e.mouse.y = numeric_cast<sint32>(floor(ypos));
			e.mouse.wheel = numeric_cast<sint8>(yoffset);
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowResizeCallback(GLFWwindow *w, int width, int height)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Resize;
			e.modifiers = getKeyModifiers(w);
			e.point.x = width;
			e.point.y = height;
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowMoveCallback(GLFWwindow *w, int xpos, int ypos)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Move;
			e.modifiers = getKeyModifiers(w);
			e.point.x = xpos;
			e.point.y = ypos;
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowIconifiedCallback(GLFWwindow *w, int iconified)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			if (iconified)
				e.type = eventStruct::eventType::Hide;
			else
				e.type = eventStruct::eventType::Show;
			e.modifiers = getKeyModifiers(w);
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowMaximizedCallback(GLFWwindow *w, int maximized)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Show; // window is visible when both maximized or restored
			e.modifiers = getKeyModifiers(w);
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowFocusCallback(GLFWwindow *w, int focused)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			if (focused)
				e.type = eventStruct::eventType::FocusGain;
			else
				e.type = eventStruct::eventType::FocusLose;
			e.modifiers = getKeyModifiers(w);
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowImpl::initializeEvents()
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

	void windowHandle::title(const string &value)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetWindowTitle(impl->window, value.c_str());
	}

	bool windowHandle::isFocused() const
	{
		windowImpl *impl = (windowImpl*)this;
#ifdef GCHL_WINDOWS_THREAD
		return impl->focus;
#else
		return glfwGetWindowAttrib(impl->window, GLFW_FOCUSED);
#endif
	}

	bool windowHandle::isFullscreen() const
	{
		windowImpl *impl = (windowImpl*)this;
		return !!glfwGetWindowMonitor(impl->window);
	}

	bool windowHandle::isMaximized() const
	{
		windowImpl *impl = (windowImpl*)this;
		return !!glfwGetWindowAttrib(impl->window, GLFW_MAXIMIZED);
	}

	bool windowHandle::isWindowed() const
	{
		return !isHidden() && !isMinimized() && !isMaximized() && !isFullscreen();
	}

	bool windowHandle::isMinimized() const
	{
		windowImpl *impl = (windowImpl*)this;
		return !!glfwGetWindowAttrib(impl->window, GLFW_ICONIFIED);
	}

	bool windowHandle::isHidden() const
	{
		windowImpl *impl = (windowImpl*)this;
		return !glfwGetWindowAttrib(impl->window, GLFW_VISIBLE);
	}

	bool windowHandle::isVisible() const
	{
		return !isHidden() && !isMinimized();
	}

	namespace
	{
		void normalizeWindow(windowImpl *impl, windowFlags flags)
		{
			if (impl->isFullscreen())
				glfwSetWindowMonitor(impl->window, nullptr, 100, 100, 800, 600, 0);
			if (impl->isHidden())
				glfwShowWindow(impl->window);
			if (impl->isMinimized() || impl->isMaximized())
				glfwRestoreWindow(impl->window);
			glfwSetWindowAttrib(impl->window, GLFW_DECORATED, any(flags & windowFlags::Border));
			glfwSetWindowAttrib(impl->window, GLFW_RESIZABLE, any(flags & windowFlags::Resizeable));
		}
	}

	void windowHandle::setFullscreen(const ivec2 &resolution, uint32 frequency, const string &deviceId)
	{
		windowImpl *impl = (windowImpl*)this;
		normalizeWindow(impl, windowFlags::None);
		GLFWmonitor *m = getMonitorById(deviceId);
		const GLFWvidmode *v = glfwGetVideoMode(m);
		glfwSetWindowMonitor(impl->window, m, 0, 0,
			resolution.x == 0 ? v->width : resolution.x,
			resolution.y == 0 ? v->height : resolution.y,
			frequency == 0 ? glfwGetVideoMode(m)->refreshRate : frequency);
	}

	void windowHandle::setMaximized()
	{
		windowImpl *impl = (windowImpl*)this;
		normalizeWindow(impl, windowFlags::Border | windowFlags::Resizeable);
		glfwMaximizeWindow(impl->window);
	}

	void windowHandle::setWindowed(windowFlags flags)
	{
		windowImpl *impl = (windowImpl*)this;
		normalizeWindow(impl, flags);
	}

	void windowHandle::setMinimized()
	{
		windowImpl *impl = (windowImpl*)this;
		normalizeWindow(impl, windowFlags::Resizeable);
		glfwIconifyWindow(impl->window);
	}

	void windowHandle::setHidden()
	{
		windowImpl *impl = (windowImpl*)this;
		normalizeWindow(impl, windowFlags::None);
		glfwHideWindow(impl->window);
	}

	bool windowHandle::mouseVisible() const
	{
		windowImpl *impl = (windowImpl*)this;
		return glfwGetInputMode(impl->window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
	}

	void windowHandle::mouseVisible(bool value)
	{
		windowImpl *impl = (windowImpl*)this;
		if (value)
			glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	ivec2 windowHandle::mousePosition() const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->currentMousePosition;
	}

	void windowHandle::mousePosition(const ivec2 &tmp)
	{
		windowImpl *impl = (windowImpl*)this;
#ifdef GCHL_WINDOWS_THREAD
		scopeLock<syncMutex> l(impl->eventsMutex);
		impl->absoluteMouseSets.push_back(tmp);
		const ivec2 &c = impl->currentMousePosition;
		impl->relativeMouseOffsets.push_back(ivec2(tmp.x - c.x, tmp.y - c.y));
#else
		glfwSetCursorPos(impl->window, tmp.x, tmp.y);
#endif // GCHL_WINDOWS_THREAD
	}

	mouseButtonsFlags windowHandle::mouseButtons() const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateButtons;
	}

	modifiersFlags windowHandle::keyboardModifiers() const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateMods;
	}

	bool windowHandle::keyboardKey(uint32 key) const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateKeys.count(key);
	}

	bool windowHandle::keyboardScanCode(uint32 code) const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateCodes.count(code);
	}

	void windowHandle::processEvents()
	{
		windowImpl *impl = (windowImpl*)this;
#ifndef GCHL_WINDOWS_THREAD
		{
			scopeLock<syncMutex> l(windowsMutex());
			glfwPollEvents();
		}
#endif
		{
			scopeLock<syncMutex> l(impl->eventsMutex);
			impl->eventsQueueNoLock.insert(impl->eventsQueueNoLock.end(), impl->eventsQueueLocked.begin(), impl->eventsQueueLocked.end());
			impl->eventsQueueLocked.clear();
		}
		while (!impl->eventsQueueNoLock.empty())
		{
			eventStruct e = *impl->eventsQueueNoLock.begin();
			impl->eventsQueueNoLock.erase(impl->eventsQueueNoLock.begin());
			impl->stateMods = e.modifiers;
			switch (e.type)
			{
			case eventStruct::eventType::Close:
				impl->events.windowClose.dispatch();
				break;
			case eventStruct::eventType::KeyPress:
				impl->stateKeys.insert(e.key.key);
				impl->stateCodes.insert(e.key.scancode);
				impl->events.keyPress.dispatch(e.key.key, e.key.scancode, e.modifiers);
				impl->events.keyRepeat.dispatch(e.key.key, e.key.scancode, e.modifiers);
				break;
			case eventStruct::eventType::KeyRepeat:
				impl->events.keyRepeat.dispatch(e.key.key, e.key.scancode, e.modifiers);
				break;
			case eventStruct::eventType::KeyRelease:
				impl->events.keyRelease.dispatch(e.key.key, e.key.scancode, e.modifiers);
				impl->stateKeys.erase(e.key.key);
				impl->stateCodes.erase(e.key.scancode);
				break;
			case eventStruct::eventType::KeyChar:
				impl->events.keyChar.dispatch(e.codepoint);
				break;
			case eventStruct::eventType::MouseMove:
				impl->events.mouseMove.dispatch(e.mouse.buttons, e.modifiers, impl->processMousePositionEvent(ivec2(e.mouse.x, e.mouse.y)));
				break;
			case eventStruct::eventType::MousePress:
				impl->stateButtons |= e.mouse.buttons;
				impl->events.mousePress.dispatch(e.mouse.buttons, e.modifiers, impl->processMousePositionEvent(ivec2(e.mouse.x, e.mouse.y)));
				break;
			case eventStruct::eventType::MouseDouble:
				impl->events.mouseDouble.dispatch(e.mouse.buttons, e.modifiers, impl->processMousePositionEvent(ivec2(e.mouse.x, e.mouse.y)));
				break;
			case eventStruct::eventType::MouseRelease:
				impl->events.mouseRelease.dispatch(e.mouse.buttons, e.modifiers, impl->processMousePositionEvent(ivec2(e.mouse.x, e.mouse.y)));
				impl->stateButtons &= ~e.mouse.buttons;
				break;
			case eventStruct::eventType::MouseWheel:
				impl->events.mouseWheel.dispatch(e.mouse.wheel, e.modifiers, impl->processMousePositionEvent(ivec2(e.mouse.x, e.mouse.y)));
				break;
			case eventStruct::eventType::Resize:
				impl->events.windowResize.dispatch(ivec2(e.point.x, e.point.y));
				break;
			case eventStruct::eventType::Move:
				impl->events.windowMove.dispatch(ivec2(e.point.x, e.point.y));
				break;
			case eventStruct::eventType::Show:
				impl->events.windowShow.dispatch();
				break;
			case eventStruct::eventType::Hide:
				impl->events.windowHide.dispatch();
				break;
			case eventStruct::eventType::FocusGain:
				impl->focus = true;
				impl->events.focusGain.dispatch();
				break;
			case eventStruct::eventType::FocusLose:
				impl->focus = false;
				impl->events.focusLose.dispatch();
				break;
#ifdef GCHL_WINDOWS_THREAD
			case eventStruct::eventType::DropRelativeMouseOffset:
				impl->relativeMouseOffsets.erase(impl->relativeMouseOffsets.begin());
				break;
#endif
			default:
				CAGE_THROW_CRITICAL(exception, "invalid event type");
			}
		}
	}

	ivec2 windowHandle::resolution() const
	{
		windowImpl *impl = (windowImpl*)this;
		int w = 0, h = 0;
		glfwGetFramebufferSize(impl->window, &w, &h);
		return ivec2(w, h);
	}

	float windowHandle::contentScaling() const
	{
		windowImpl *impl = (windowImpl*)this;
		float y = 1;
		glfwGetWindowContentScale(impl->window, nullptr, &y);
		return y;
	}

	ivec2 windowHandle::windowedSize() const
	{
		windowImpl *impl = (windowImpl*)this;
		int w = 0, h = 0;
		glfwGetWindowSize(impl->window, &w, &h);
		return ivec2(w, h);
	}

	void windowHandle::windowedSize(const ivec2 &tmp)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetWindowSize(impl->window, tmp.x, tmp.y);
	}

	ivec2 windowHandle::windowedPosition() const
	{
		windowImpl *impl = (windowImpl*)this;
		int x = 0, y = 0;
		glfwGetWindowPos(impl->window, &x, &y);
		return ivec2(x, y);
	}

	void windowHandle::windowedPosition(const ivec2 &tmp)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetWindowPos(impl->window, tmp.x, tmp.y);
	}

	void windowHandle::makeCurrent()
	{
		windowImpl *impl = (windowImpl*)this;
		glfwMakeContextCurrent(impl->window);
		setCurrentContext(this);
	}

	void windowHandle::makeNotCurrent()
	{
		glfwMakeContextCurrent(nullptr);
		setCurrentContext(nullptr);
	}

	void windowHandle::swapBuffers()
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSwapBuffers(impl->window);
	}

	holder<windowHandle> newWindow(windowHandle *shareContext)
	{
		return detail::systemArena().createImpl<windowHandle, windowImpl>(shareContext);
	}

	void windowEventListeners::attachAll(windowHandle *window, sint32 order)
	{
#define GCHL_GENERATE(T) if(window) T.attach(window->events.T, order); else T.detach();
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, windowClose, windowShow, windowHide, windowMove, windowResize, mouseMove, mousePress, mouseDouble, mouseRelease, mouseWheel, focusGain, focusLose, keyPress, keyRelease, keyRepeat, keyChar));
#undef GCHL_GENERATE
	}
}
