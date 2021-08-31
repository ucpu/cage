#ifndef guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC
#define guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC

#include <cage-core/events.h>

#include "core.h"

namespace cage
{
	enum class WindowFlags : uint32
	{
		None = 0,
		Resizeable = 1 << 0,
		Border = 1 << 1,
	};

	class CAGE_ENGINE_API Window : private Immovable
	{
	public:
		struct
		{
			EventDispatcher<bool()> windowClose, windowShow, windowHide;
			EventDispatcher<bool(const Vec2i &)> windowMove, windowResize;
			EventDispatcher<bool(MouseButtonsFlags, ModifiersFlags, const Vec2i &)> mouseMove, mousePress, mouseDouble, mouseRelease;
			EventDispatcher<bool(sint32, ModifiersFlags, const Vec2i &)> mouseWheel;
			EventDispatcher<bool()> focusGain, focusLose;
			EventDispatcher<bool(uint32, ModifiersFlags)> keyPress, keyRelease, keyRepeat;
			EventDispatcher<bool(uint32)> keyChar;
		} events;

		void processEvents();

		//string title() const;
		void title(const String &value);

		bool isFocused() const;
		bool isFullscreen() const;
		bool isMaximized() const;
		bool isWindowed() const; // not hidden, not minimized, not maximized and not fullscreen
		bool isMinimized() const;
		bool isHidden() const;
		bool isVisible() const; // not hidden and not minimized

		void setFullscreen(const Vec2i &resolution, uint32 frequency = 0, const String &screenId = "");
		void setMaximized();
		void setWindowed(WindowFlags flags = WindowFlags::Border | WindowFlags::Resizeable);
		void setMinimized();
		void setHidden();

		bool mouseVisible() const;
		void mouseVisible(bool value);

		Vec2i mousePosition() const;
		void mousePosition(const Vec2i &);
		MouseButtonsFlags mouseButtons() const;

		ModifiersFlags keyboardModifiers() const;
		bool keyboardKey(uint32 key) const;

		Vec2i resolution() const;
		Real contentScaling() const;
		String screenId() const;

		Vec2i windowedSize() const;
		void windowedSize(const Vec2i &);
		Vec2i windowedPosition() const;
		void windowedPosition(const Vec2i &);

		void makeCurrent();
		void makeNotCurrent();
		void swapBuffers();

		Delegate<void(uint32, uint32, uint32, uint32, const char*)> debugOpenglErrorCallback;
	};

	CAGE_ENGINE_API Holder<Window> newWindow(Window *shareContext = nullptr);

	struct CAGE_ENGINE_API WindowEventListeners
	{
		EventListener<bool()> windowClose, windowShow, windowHide;
		EventListener<bool(const Vec2i &)> windowMove, windowResize;
		EventListener<bool(MouseButtonsFlags, ModifiersFlags, const Vec2i &)> mouseMove, mousePress, mouseDouble, mouseRelease;
		EventListener<bool(sint32, ModifiersFlags, const Vec2i &)> mouseWheel;
		EventListener<bool()> focusGain, focusLose;
		EventListener<bool(uint32, ModifiersFlags)> keyPress, keyRelease, keyRepeat;
		EventListener<bool(uint32)> keyChar;

		void attachAll(Window *window, sint32 order = 0);
	};
}

#endif
