namespace cage
{
	struct CAGE_API elementLayoutStruct
	{
		struct CAGE_API textureUvStruct
		{
			struct CAGE_API textureUvOiStruct
			{
				vec4 outer;
				vec4 inner;
			} defal, focus, hover, disab;
		} textureUv;
		vec4 margin; // left, top, right, bottom
		vec4 border;
		vec4 padding;
		vec2 defaultSize;
		unitsModeEnum marginUnits;
		unitsModeEnum borderUnits;
		unitsModeEnum paddingUnits;
		unitsModeEnum defaultSizeUnits;
	};

	template struct CAGE_API eventDispatcher<bool(uint32)>;

	class CAGE_API guiClass
	{
	public:
		entityManagerClass *entities();
		assetManagerClass *assets();

		void graphicInitialize(windowClass *graphicContext);
		void graphicFinalize();
		void soundInitialize(soundContextClass *soundContext);
		void soundFinalize();

		void controlEmit();
		void graphicPrepare(uint64 time);
		void graphicDispatch();
		void soundRender();
		void setCursorPosition(const pointStruct &point);

		/*
		void loadScreen(const void *buffer, uintPtr size);
		void loadScreen(const memoryBuffer &buffer);
		void saveScreen(void *buffer, uintPtr &size) const;
		void saveScreen(memoryBuffer &buffer) const;
		*/

		void setFocus(uint32 control);
		uint32 getFocus() const;

		bool windowResize(windowClass *win, const pointStruct &);
		bool mousePress(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		bool mouseRelease(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		bool mouseMove(windowClass *win, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		bool mouseWheel(windowClass *win, sint8 wheel, modifiersFlags modifiers, const pointStruct &point);
		bool keyPress(windowClass *win, uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyRepeat(windowClass *win, uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyRelease(windowClass *win, uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyChar(windowClass *win, uint32 key);
		void handleWindowEvents(windowClass *window) const;

		eventDispatcher<bool(uint32)> genericEvent;

		elementLayoutStruct elements[(uint32)elementTypeEnum::TotalElements];
		componentsStruct components;
		vec3 defaultFontColor;
		uint32 defaultFontName;
		uint32 skinName;
	};

	struct CAGE_API guiCreateConfig
	{
		assetManagerClass *assetManager;
		entityManagerCreateConfig *entitiesConfig;
		uintPtr memoryLimitCache;
		uintPtr memoryLimitPrepare;
		guiCreateConfig();
	};

	CAGE_API holder<guiClass> newGui(const guiCreateConfig &config);

	namespace detail
	{
		CAGE_API void guiElementsLayoutTemplateExport(elementLayoutStruct elements[(uint32)elementTypeEnum::TotalElements], uint32 resolution, const string &path);
	}
}
