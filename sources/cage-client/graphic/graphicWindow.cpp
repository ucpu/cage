#include <vector>
#include <atomic>

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/log.h>
#include <cage-core/concurrent.h>

#define CAGE_EXPORT
#include <cage-core/core/macro/api.h>
#include <cage-client/core.h>
#include <cage-client/graphic.h>
#include <cage-client/opengl.h>
#include "private.h"
#include <GLFW/glfw3.h>

namespace cage
{
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

		void handleGlfwError(int, const char *message)
		{
			CAGE_LOG(severityEnum::Error, "glfw", message);
		}

		class glfwInitializerClass
		{
		public:
			glfwInitializerClass()
			{
				glfwSetErrorCallback(&handleGlfwError);
				if (!glfwInit())
					CAGE_THROW_CRITICAL(exception, "failed to initialize glfw");
			}

			~glfwInitializerClass()
			{
				glfwTerminate();
			}
		} glfwInitializerInstance;

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
			union
			{
				struct
				{
					sint32 x, y;
					modifiersFlags modifiers;
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
					modifiersFlags modifiers;
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
			windowClass *shareContext;
			holder<mutexClass> eventsMutex;
			std::vector<eventStruct> eventsQueueLocked;
			std::vector<eventStruct> eventsQueueNoLock;
			GLFWwindow *window;
			bool focus;

#ifdef CAGE_SYSTEM_WINDOWS
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

			windowImpl(windowClass *shareContext) : shareContext(shareContext), eventsMutex(newMutex()), focus(true)
			{
#ifdef CAGE_SYSTEM_WINDOWS
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
						CAGE_THROW_ERROR(graphicException, "gladLoadGl", 0);
				}
				openglContextInitializeGeneral(this);
			}

			~windowImpl()
			{
#ifdef CAGE_SYSTEM_WINDOWS
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
#endif // CAGE_DEBUG
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
		};

		void windowCloseEvent(GLFWwindow *w)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Close;
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
			e.key.key = key;
			e.key.scancode = scancode;
			e.key.modifiers = modifiersFlags::None
				| ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT ? modifiersFlags::Shift : modifiersFlags::None)
				| ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL ? modifiersFlags::Ctrl : modifiersFlags::None)
				| ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT ? modifiersFlags::Alt : modifiersFlags::None);
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowCharCallback(GLFWwindow *w, unsigned int codepoint)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::KeyChar;
			e.codepoint = codepoint;
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
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
			return r;
		}

		void windowMouseMove(GLFWwindow *w, double xpos, double ypos)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::MouseMove;
			e.mouse.x = numeric_cast<sint32>(floor(xpos));
			e.mouse.y = numeric_cast<sint32>(floor(ypos));
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT))
				e.mouse.buttons |= mouseButtonsFlags::Left;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT))
				e.mouse.buttons |= mouseButtonsFlags::Right;
			if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE))
				e.mouse.buttons |= mouseButtonsFlags::Middle;
			e.mouse.modifiers = getKeyModifiers(w);
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
			e.mouse.modifiers = modifiersFlags::None
				| ((mods & GLFW_MOD_SHIFT) == GLFW_MOD_SHIFT ? modifiersFlags::Shift : modifiersFlags::None)
				| ((mods & GLFW_MOD_CONTROL) == GLFW_MOD_CONTROL ? modifiersFlags::Ctrl : modifiersFlags::None)
				| ((mods & GLFW_MOD_ALT) == GLFW_MOD_ALT ? modifiersFlags::Alt : modifiersFlags::None);
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowMouseWheel(GLFWwindow *w, double, double yoffset)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::MouseWheel;
			double xpos, ypos;
			glfwGetCursorPos(w, &xpos, &ypos);
			e.mouse.x = numeric_cast<sint32>(floor(xpos));
			e.mouse.y = numeric_cast<sint32>(floor(ypos));
			e.mouse.modifiers = getKeyModifiers(w);
			e.mouse.wheel = numeric_cast<sint8>(yoffset);
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowResizeCallback(GLFWwindow *w, int width, int height)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Resize;
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
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueLocked.push_back(e);
		}

		void windowPaintCallback(GLFWwindow *w)
		{
			windowImpl *impl = (windowImpl*)glfwGetWindowUserPointer(w);
			eventStruct e;
			e.type = eventStruct::eventType::Paint;
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
#ifdef CAGE_SYSTEM_WINDOWS
		return impl->focus;
#else
		return glfwGetWindowAttrib(impl->window, GLFW_FOCUSED);
#endif // CAGE_SYSTEM_WINDOWS
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

	void windowClass::processEvents()
	{
		windowImpl *impl = (windowImpl*)this;
#ifndef CAGE_SYSTEM_WINDOWS
		{
			scopeLock<mutexClass> l(windowsMutex());
			glfwPollEvents();
		}
#endif // !CAGE_SYSTEM_WINDOWS
		{
			scopeLock<mutexClass> l(impl->eventsMutex);
			impl->eventsQueueNoLock.insert(impl->eventsQueueNoLock.end(), impl->eventsQueueLocked.begin(), impl->eventsQueueLocked.end());
			impl->eventsQueueLocked.clear();
		}
		while (!impl->eventsQueueNoLock.empty())
		{
			eventStruct e = *impl->eventsQueueNoLock.begin();
			impl->eventsQueueNoLock.erase(impl->eventsQueueNoLock.begin());
			switch (e.type)
			{
			case eventStruct::eventType::Close:
				impl->events.windowClose.dispatch(this);
				break;
			case eventStruct::eventType::KeyPress:
				impl->events.keyPress.dispatch(this, e.key.key, e.key.scancode, e.key.modifiers);
				impl->events.keyRepeat.dispatch(this, e.key.key, e.key.scancode, e.key.modifiers);
				break;
			case eventStruct::eventType::KeyRepeat:
				impl->events.keyRepeat.dispatch(this, e.key.key, e.key.scancode, e.key.modifiers);
				break;
			case eventStruct::eventType::KeyRelease:
				impl->events.keyRelease.dispatch(this, e.key.key, e.key.scancode, e.key.modifiers);
				break;
			case eventStruct::eventType::KeyChar:
				impl->events.keyChar.dispatch(this, e.codepoint);
				break;
			case eventStruct::eventType::MouseMove:
				impl->events.mouseMove.dispatch(this, e.mouse.buttons, e.mouse.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::MousePress:
				impl->events.mousePress.dispatch(this, e.mouse.buttons, e.mouse.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::MouseDouble:
				impl->events.mouseDouble.dispatch(this, e.mouse.buttons, e.mouse.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::MouseRelease:
				impl->events.mouseRelease.dispatch(this, e.mouse.buttons, e.mouse.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::MouseWheel:
				impl->events.mouseWheel.dispatch(this, e.mouse.wheel, e.mouse.modifiers, pointStruct(e.mouse.x, e.mouse.y));
				break;
			case eventStruct::eventType::Resize:
				impl->events.windowResize.dispatch(this, pointStruct(e.point.x, e.point.y));
				break;
			case eventStruct::eventType::Move:
				impl->events.windowMove.dispatch(this, pointStruct(e.point.x, e.point.y));
				break;
			case eventStruct::eventType::Show:
				impl->events.windowShow.dispatch(this);
				break;
			case eventStruct::eventType::Hide:
				impl->events.windowHide.dispatch(this);
				break;
			case eventStruct::eventType::Paint:
				impl->events.windowPaint.dispatch(this);
				break;
			case eventStruct::eventType::FocusGain:
				impl->focus = true;
				impl->events.focusGain.dispatch(this);
				break;
			case eventStruct::eventType::FocusLose:
				impl->focus = false;
				impl->events.focusLose.dispatch(this);
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
#define GCHL_GENERATE(T) if (window) T.attach(window->events.T); else T.detach();
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, applicationQuit, windowClose, windowShow, windowHide, windowPaint, windowMove, windowResize, mousePress, mouseDouble, mouseRelease, mouseMove, mouseWheel, focusGain, focusLose, keyPress, keyRepeat, keyRelease, keyChar));
#undef GCHL_GENERATE
	}
}
