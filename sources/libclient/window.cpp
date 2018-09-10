#include <atomic>
#include <vector>
#include <set>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/window.h>
#include <cage-client/graphics.h>
#include <cage-client/opengl.h>
#include "graphics/private.h"
#include <GLFW/glfw3.h>

//#define GCHL_WINDOWS_THREAD

namespace cage
{
	namespace
	{
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
		class mutexInitClass
		{
		public:
			mutexInitClass()
			{
				m = newMutex();
			}
			holder<mutexClass> m;
		};

		mutexClass *windowsMutex()
		{
			static mutexInitClass *m = new mutexInitClass(); // intentional leak
			return m->m.get();
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
				Paint,
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

		class windowImpl : public windowClass
		{
		public:
			uint64 lastMouseButtonPressTimes[5]; // unused, left, right, unused, middle
			windowClass *shareContext;
			holder<mutexClass> eventsMutex;
			std::vector<eventStruct> eventsQueueLocked;
			std::vector<eventStruct> eventsQueueNoLock;
			std::set<uint32> stateKeys, stateCodes;
			modifiersFlags stateMods;
			mouseButtonsFlags stateButtons;
			GLFWwindow *window;
			bool focus;

#ifdef GCHL_WINDOWS_THREAD
			holder<threadClass> windowThread;
			holder<semaphoreClass> windowSemaphore;
			std::atomic<bool> stopping;

			void treadEntry()
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
						scopeLock<mutexClass> l(windowsMutex());
						try
						{
							glfwPollEvents();
						}
						catch (...)
						{
							CAGE_LOG(severityEnum::Warning, "glfw", "an exception occured in event processing");
						}
					}
					threadSleep(5000);
				}
				finalizeWindow();
			}
#endif

			windowImpl(windowClass *shareContext) : lastMouseButtonPressTimes{0,0,0,0,0}, shareContext(shareContext), eventsMutex(newMutex()), stateMods(modifiersFlags::None), stateButtons(mouseButtonsFlags::None), window(nullptr), focus(true)
			{
				cageGlfwInitializeFunc();

#ifdef GCHL_WINDOWS_THREAD
				stopping = false;
				windowSemaphore = newSemaphore(0, 1);
				windowThread = newThread(delegate<void()>().bind<windowImpl, &windowImpl::treadEntry>(this), "window");
				windowSemaphore->lock();
				if (stopping)
					CAGE_THROW_ERROR(exception, "failed to initialize window");
#else
				initializeWindow();
#endif

				// initialize opengl
				makeCurrent();
				{
					// initialize opengl functions (gl loader)
					if (!gladLoadGL())
						CAGE_THROW_ERROR(graphicsException, "gladLoadGl", 0);
				}
				openglContextInitializeGeneral(this);
			}

			~windowImpl()
			{
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
				scopeLock<mutexClass> lock(windowsMutex());
				glfwDefaultWindowHints();
				glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
				glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
				glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,
#ifdef CAGE_DEBUG
					GLFW_TRUE
#else
					GLFW_FALSE
#endif
				);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				window = glfwCreateWindow(1, 1, "Cage GLFW Window", NULL, shareContext ? ((windowImpl*)shareContext)->window : nullptr);
				if (!window)
					CAGE_THROW_ERROR(exception, "failed to create glfw window");
				glfwSetWindowUserPointer(window, this);
				initializeEvents();
			}

			void finalizeWindow()
			{
				scopeLock<mutexClass> lock(windowsMutex());
				glfwDestroyWindow(window);
				window = nullptr;
			}

			void initializeEvents();

			bool determineMouseDoubleClick(const eventStruct &e)
			{
				if (e.type != eventStruct::eventType::MousePress)
					return false;
				CAGE_ASSERT_RUNTIME((uint32)e.mouse.buttons < sizeof(lastMouseButtonPressTimes) / sizeof(lastMouseButtonPressTimes[0]));
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

		void windowCloseEvent(GLFWwindow *w)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Close;
			e.modifiers = getKeyModifiers(w);
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowCharCallback(GLFWwindow *w, unsigned int codepoint)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::KeyChar;
			e.modifiers = getKeyModifiers(w);
			e.codepoint = codepoint;
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowPaintCallback(GLFWwindow *w)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Paint;
			e.modifiers = getKeyModifiers(w);
			scopeLock<mutexClass> l(impl->eventsMutex);
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
			glfwSetWindowFocusCallback(window, &windowFocusCallback);
			glfwSetWindowRefreshCallback(window, &windowPaintCallback);
		}
	}

	void windowClass::title(const string &value)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetWindowTitle(impl->window, value.c_str());
	}

	bool windowClass::isFocused() const
	{
		windowImpl *impl = (windowImpl*)this;
#ifdef GCHL_WINDOWS_THREAD
		return impl->focus;
#else
		return glfwGetWindowAttrib(impl->window, GLFW_FOCUSED);
#endif
	}

	bool windowClass::isFullscreen() const
	{
		windowImpl *impl = (windowImpl*)this;
		return !!glfwGetWindowMonitor(impl->window);
	}

	void windowClass::modeSetFullscreen(const pointStruct &resolution, uint32 frequency, const string &deviceId)
	{
		windowImpl *impl = (windowImpl*)this;
		GLFWmonitor *m = getMonitorById(deviceId);
		const GLFWvidmode *v = glfwGetVideoMode(m);
		glfwSetWindowMonitor(impl->window, m, 0, 0,
			resolution.x == 0 ? v->width : resolution.x,
			resolution.y == 0 ? v->height : resolution.y,
			frequency == 0 ? glfwGetVideoMode(m)->refreshRate : frequency);
	}

	void windowClass::modeSetWindowed(windowFlags flags)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetWindowMonitor(impl->window, nullptr, 100, 100, 800, 600, 0);
		glfwSetWindowAttrib(impl->window, GLFW_DECORATED, (flags & windowFlags::Border) == windowFlags::Border);
		glfwSetWindowAttrib(impl->window, GLFW_RESIZABLE, (flags & windowFlags::Resizeable) == windowFlags::Resizeable);
	}

	bool windowClass::mouseVisible() const
	{
		windowImpl *impl = (windowImpl*)this;
		return glfwGetInputMode(impl->window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
	}

	void windowClass::mouseVisible(bool value)
	{
		windowImpl *impl = (windowImpl*)this;
		if (value)
			glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(impl->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	pointStruct windowClass::mousePosition() const
	{
		windowImpl *impl = (windowImpl*)this;
		double x = 0, y = 0;
		glfwGetCursorPos(impl->window, &x, &y);
		return pointStruct((sint32)floor(x).value, (sint32)floor(y).value);
	}

	void windowClass::mousePosition(const pointStruct &tmp)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetCursorPos(impl->window, tmp.x, tmp.y);
	}

	mouseButtonsFlags windowClass::mouseButtons() const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateButtons;
	}

	modifiersFlags windowClass::keyboardModifiers() const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateMods;
	}

	bool windowClass::keyboardKey(uint32 key) const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateKeys.count(key);
	}

	bool windowClass::keyboardScanCode(uint32 code) const
	{
		windowImpl *impl = (windowImpl*)this;
		return impl->stateCodes.count(code);
	}

	void windowClass::processEvents()
	{
		windowImpl *impl = (windowImpl*)this;
#ifndef GCHL_WINDOWS_THREAD
		{
			scopeLock<mutexClass> l(windowsMutex());
			glfwPollEvents();
		}
#endif
		{
			scopeLock<mutexClass> l(impl->eventsMutex);
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
				impl->events.mouseMove.dispatch(e.mouse.buttons, e.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::MousePress:
				impl->stateButtons |= e.mouse.buttons;
				impl->events.mousePress.dispatch(e.mouse.buttons, e.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::MouseDouble:
				impl->events.mouseDouble.dispatch(e.mouse.buttons, e.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::MouseRelease:
				impl->events.mouseRelease.dispatch(e.mouse.buttons, e.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				impl->stateButtons &= ~e.mouse.buttons;
				break;
			case eventStruct::eventType::MouseWheel:
				impl->events.mouseWheel.dispatch(e.mouse.wheel, e.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::Resize:
				impl->events.windowResize.dispatch(pointStruct(e.point.x, e.point.y));
				break;
			case eventStruct::eventType::Move:
				impl->events.windowMove.dispatch(pointStruct(e.point.x, e.point.y));
				break;
			case eventStruct::eventType::Show:
				impl->events.windowShow.dispatch();
				break;
			case eventStruct::eventType::Hide:
				impl->events.windowHide.dispatch();
				break;
			case eventStruct::eventType::Paint:
				impl->events.windowPaint.dispatch();
				break;
			case eventStruct::eventType::FocusGain:
				impl->focus = true;
				impl->events.focusGain.dispatch();
				break;
			case eventStruct::eventType::FocusLose:
				impl->focus = false;
				impl->events.focusLose.dispatch();
				break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid event type");
			}
		}
	}

	pointStruct windowClass::resolution() const
	{
		windowImpl *impl = (windowImpl*)this;
		int w = 0, h = 0;
		glfwGetWindowSize(impl->window, &w, &h);
		return pointStruct(w, h);
	}

	pointStruct windowClass::windowedSize() const
	{
		windowImpl *impl = (windowImpl*)this;
		int w = 0, h = 0;
		glfwGetWindowSize(impl->window, &w, &h);
		return pointStruct(w, h);
	}

	void windowClass::windowedSize(const pointStruct &tmp)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetWindowSize(impl->window, tmp.x, tmp.y);
	}

	pointStruct windowClass::windowedPosition() const
	{
		windowImpl *impl = (windowImpl*)this;
		int x = 0, y = 0;
		glfwGetWindowPos(impl->window, &x, &y);
		return pointStruct(x, y);
	}

	void windowClass::windowedPosition(const pointStruct &tmp)
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSetWindowPos(impl->window, tmp.x, tmp.y);
	}

	void windowClass::makeCurrent()
	{
		windowImpl *impl = (windowImpl*)this;
		glfwMakeContextCurrent(impl->window);
		setCurrentContext(this);
	}

	void windowClass::makeNotCurrent()
	{
		glfwMakeContextCurrent(nullptr);
		setCurrentContext(nullptr);
	}

	void windowClass::swapBuffers()
	{
		windowImpl *impl = (windowImpl*)this;
		glfwSwapBuffers(impl->window);
	}

	holder<windowClass> newWindow(windowClass *shareContext)
	{
		return detail::systemArena().createImpl<windowClass, windowImpl>(shareContext);
	}

	void windowEventListeners::attachAll(windowClass *window)
	{
#define GCHL_GENERATE(T) if(window) T.attach(window->events.T); else T.detach();
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, windowClose, windowShow, windowHide, windowPaint, windowMove, windowResize, mouseMove, mousePress, mouseDouble, mouseRelease, mouseWheel, focusGain, focusLose, keyPress, keyRelease, keyRepeat, keyChar));
#undef GCHL_GENERATE
	}
}
