#ifndef guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC
#define guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC

namespace cage
{
	class CAGE_API windowHandle : private Immovable
	{
	public:
		struct
		{
			EventDispatcher<bool()> windowClose, windowShow, windowHide;
			EventDispatcher<bool(const ivec2 &)> windowMove, windowResize;
			EventDispatcher<bool(mouseButtonsFlags, modifiersFlags, const ivec2 &)> mouseMove, mousePress, mouseDouble, mouseRelease;
			EventDispatcher<bool(sint32, modifiersFlags, const ivec2 &)> mouseWheel;
			EventDispatcher<bool()> focusGain, focusLose;
			EventDispatcher<bool(uint32, uint32, modifiersFlags)> keyPress, keyRelease, keyRepeat;
			EventDispatcher<bool(uint32)> keyChar;
		} events;

		void processEvents();

		//string title() const;
		void title(const string &value);

		bool isFocused() const;
		bool isFullscreen() const;
		bool isMaximized() const;
		bool isWindowed() const; // not hidden, not minimized, not maximized and not fullscreen
		bool isMinimized() const;
		bool isHidden() const;
		bool isVisible() const; // not hidden and not minimized

		void setFullscreen(const ivec2 &resolution, uint32 frequency = 0, const string &deviceId = "");
		void setMaximized();
		void setWindowed(windowFlags flags = windowFlags::Border | windowFlags::Resizeable);
		void setMinimized();
		void setHidden();

		bool mouseVisible() const;
		void mouseVisible(bool value);

		ivec2 mousePosition() const;
		void mousePosition(const ivec2 &);
		mouseButtonsFlags mouseButtons() const;

		modifiersFlags keyboardModifiers() const;
		bool keyboardKey(uint32 key) const;
		bool keyboardScanCode(uint32 code) const;

		ivec2 resolution() const;
		float contentScaling() const;

		ivec2 windowedSize() const;
		void windowedSize(const ivec2 &);
		ivec2 windowedPosition() const;
		void windowedPosition(const ivec2 &);

		void makeCurrent();
		void makeNotCurrent();
		void swapBuffers();

		Delegate<void(uint32, uint32, uint32, uint32, const char*)> debugOpenglErrorCallback;
	};

	CAGE_API Holder<windowHandle> newWindow(windowHandle *shareContext = nullptr);

	struct CAGE_API windowEventListeners
	{
		EventListener<bool()> windowClose, windowShow, windowHide;
		EventListener<bool(const ivec2 &)> windowMove, windowResize;
		EventListener<bool(mouseButtonsFlags, modifiersFlags, const ivec2 &)> mouseMove, mousePress, mouseDouble, mouseRelease;
		EventListener<bool(sint32, modifiersFlags, const ivec2 &)> mouseWheel;
		EventListener<bool()> focusGain, focusLose;
		EventListener<bool(uint32, uint32, modifiersFlags)> keyPress, keyRelease, keyRepeat;
		EventListener<bool(uint32)> keyChar;

		void attachAll(windowHandle *window, sint32 order = 0);
	};
}

#endif
