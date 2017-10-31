namespace cage
{
	template struct CAGE_API delegate<void(uint32, uint32, uint32, uint32, const char*, const windowClass *)>;

	class CAGE_API windowClass
	{
	public:
		struct
		{
			eventDispatcher<bool(windowClass *)> windowClose, windowShow, windowHide, windowPaint;
			eventDispatcher<bool(windowClass *, const pointStruct &)> windowMove, windowResize;
			eventDispatcher<bool(windowClass *, mouseButtonsFlags, modifiersFlags, const pointStruct &)> mousePress, mouseDouble, mouseRelease, mouseMove;
			eventDispatcher<bool(windowClass *, sint8, modifiersFlags, const pointStruct &)> mouseWheel;
			eventDispatcher<bool(windowClass *)> focusGain, focusLose;
			eventDispatcher<bool(windowClass *, uint32, uint32, modifiersFlags)> keyPress, keyRepeat, keyRelease;
			eventDispatcher<bool(windowClass *, uint32)> keyChar;
		} events;

		void processEvents();

		//string title() const;
		void title(const string &value);

		bool isFocused() const;
		bool isFullscreen() const;
		void modeSetFullscreen(const pointStruct &resolution, uint32 frequency = 0, const string &deviceId = "");
		void modeSetWindowed(windowFlags flags = windowFlags::Border);

		pointStruct mousePosition() const;
		void mousePosition(const pointStruct &);

		bool mouseVisible() const;
		void mouseVisible(bool value);

		pointStruct resolution() const;

		pointStruct windowedSize() const;
		void windowedSize(const pointStruct &);

		pointStruct windowedPosition() const;
		void windowedPosition(const pointStruct &);

		void makeCurrent();
		void makeNotCurrent();
		void swapBuffers();

		delegate<void(uint32, uint32, uint32, uint32, const char*, const windowClass *)> debugOpenglErrorCallback;
	};

	CAGE_API holder<windowClass> newWindow(windowClass *shareContext = nullptr);

	template struct CAGE_API eventDispatcher<bool(windowClass *)>;
	template struct CAGE_API eventDispatcher<bool(windowClass *, const pointStruct &)>;
	template struct CAGE_API eventDispatcher<bool(windowClass *, mouseButtonsFlags, modifiersFlags, const pointStruct &)>;
	template struct CAGE_API eventDispatcher<bool(windowClass *, sint8, modifiersFlags, const pointStruct &)>;
	template struct CAGE_API eventDispatcher<bool(windowClass *, uint32, uint32, modifiersFlags)>;
	template struct CAGE_API eventDispatcher<bool(windowClass *, uint32)>;

	template struct CAGE_API eventListener<bool(windowClass *)>;
	template struct CAGE_API eventListener<bool(windowClass *, const pointStruct &)>;
	template struct CAGE_API eventListener<bool(windowClass *, mouseButtonsFlags, modifiersFlags, const pointStruct &)>;
	template struct CAGE_API eventListener<bool(windowClass *, sint8, modifiersFlags, const pointStruct &)>;
	template struct CAGE_API eventListener<bool(windowClass *, uint32, uint32, modifiersFlags)>;
	template struct CAGE_API eventListener<bool(windowClass *, uint32)>;

	struct CAGE_API windowEventListeners
	{
		eventListener<bool(windowClass *)> windowClose, windowShow, windowHide, windowPaint;
		eventListener<bool(windowClass *, const pointStruct &)> windowMove, windowResize;
		eventListener<bool(windowClass *, mouseButtonsFlags, modifiersFlags, const pointStruct &)> mousePress, mouseDouble, mouseRelease, mouseMove;
		eventListener<bool(windowClass *, sint8, modifiersFlags, const pointStruct &)> mouseWheel;
		eventListener<bool(windowClass *)> focusGain, focusLose;
		eventListener<bool(windowClass *, uint32, uint32, modifiersFlags)> keyPress, keyRepeat, keyRelease;
		eventListener<bool(windowClass *, uint32)> keyChar;

		void attachAll(windowClass *window);
	};

	namespace detail
	{
		CAGE_API void purgeGlShaderCache();
	}
}
