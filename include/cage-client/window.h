#ifndef guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC
#define guard_window_h_9D2B2359EB7C4009961D1CAFA13BE5FC

namespace cage
{
	class CAGE_API windowClass
	{
	public:
		struct
		{
			eventDispatcher<bool()> windowClose, windowShow, windowHide, windowPaint;
			eventDispatcher<bool(const pointStruct &)> windowMove, windowResize;
			eventDispatcher<bool(mouseButtonsFlags, modifiersFlags, const pointStruct &)> mouseMove, mousePress, mouseDouble, mouseRelease;
			eventDispatcher<bool(sint8, modifiersFlags, const pointStruct &)> mouseWheel;
			eventDispatcher<bool()> focusGain, focusLose;
			eventDispatcher<bool(uint32, uint32, modifiersFlags)> keyPress, keyRelease, keyRepeat;
			eventDispatcher<bool(uint32)> keyChar;
		} events;

		void processEvents();

		//string title() const;
		void title(const string &value);

		bool isFocused() const;
		bool isFullscreen() const;
		void modeSetFullscreen(const pointStruct &resolution, uint32 frequency = 0, const string &deviceId = "");
		void modeSetWindowed(windowFlags flags = windowFlags::Border);

		bool mouseVisible() const;
		void mouseVisible(bool value);

		pointStruct mousePosition() const;
		void mousePosition(const pointStruct &);
		mouseButtonsFlags mouseButtons() const;

		modifiersFlags keyboardModifiers() const;
		bool keyboardKey(uint32 key) const;
		bool keyboardScanCode(uint32 code) const;

		pointStruct resolution() const;

		pointStruct windowedSize() const;
		void windowedSize(const pointStruct &);

		pointStruct windowedPosition() const;
		void windowedPosition(const pointStruct &);

		void makeCurrent();
		void makeNotCurrent();
		void swapBuffers();

		delegate<void(uint32, uint32, uint32, uint32, const char*)> debugOpenglErrorCallback;
	};

	CAGE_API holder<windowClass> newWindow(windowClass *shareContext = nullptr);

	struct CAGE_API windowEventListeners
	{
		eventListener<bool()> windowClose, windowShow, windowHide, windowPaint;
		eventListener<bool(const pointStruct &)> windowMove, windowResize;
		eventListener<bool(mouseButtonsFlags, modifiersFlags, const pointStruct &)> mouseMove, mousePress, mouseDouble, mouseRelease;
		eventListener<bool(sint8, modifiersFlags, const pointStruct &)> mouseWheel;
		eventListener<bool()> focusGain, focusLose;
		eventListener<bool(uint32, uint32, modifiersFlags)> keyPress, keyRelease, keyRepeat;
		eventListener<bool(uint32)> keyChar;

		void attachAll(windowClass *window);
	};
}

#endif
