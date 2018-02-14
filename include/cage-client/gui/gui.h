namespace cage
{
	struct CAGE_API skinConfigStruct
	{
		skinElementLayoutStruct layouts[(uint32)elementTypeEnum::TotalElements];
		skinWidgetDefaultsStruct defaults;
		uint32 textureName;
		skinConfigStruct();
	};

	struct CAGE_API componentsStruct : public generalComponentsStruct, public widgetsComponentsStruct, public layoutsComponentsStruct
	{
		componentsStruct(entityManagerClass *ents);
	};

	template struct CAGE_API eventDispatcher<bool(uint32)>;
	template struct CAGE_API delegate<bool(const pointStruct&, vec2&)>;

	class CAGE_API guiClass
	{
	public:
		void graphicInitialize(windowClass *graphicContext);
		void graphicFinalize();
		void graphicRender();

		void soundInitialize(soundContextClass *soundContext);
		void soundFinalize();
		void soundRender();

		void setOutputResolution(const pointStruct &resolution, real retina = 1); // resolution: pixels; retina: how many pixels per point (1D)
		void setZoom(real zoom); // pixels per point (1D)
		pointStruct getOutputResolution() const;
		real getOutputRetina() const;
		real getZoom() const;
		void setOutputSoundBus(busClass *bus);
		busClass *getOutputSoundBus() const;
		void setFocus(uint32 widget);
		uint32 getFocus() const;
		void controlUpdate(); // must be called before processing events
		void controlEmit(); // must be called exclusively when no other threads are interacting with this gui

		bool windowResize(const pointStruct &);
		bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
		bool mouseWheel(sint8 wheel, modifiersFlags modifiers, const pointStruct &point);
		bool keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyChar(uint32 key);

		void handleWindowEvents(windowClass *window);
		void skipAllEventsUntilNextUpdate();
		pointStruct getInputResolution() const;
		delegate<bool(const pointStruct&, vec2&)> eventCoordinatesTransformer; // called from controlProcess or from any window events, it should return false to signal that the point is outside the gui, otherwise the point should be converted from window coordinate system to the gui output resolution coordinate system
		eventDispatcher<bool(uint32)> widgetEvent; // called from controlProcess or window events

		skinConfigStruct &skin(uint32 index = 0);
		const skinConfigStruct &skin(uint32 index = 0) const;

		componentsStruct &components();
		entityManagerClass *entities();
		assetManagerClass *assets();
	};

	struct CAGE_API guiCreateConfig
	{
		assetManagerClass *assetManager;
		entityManagerCreateConfig *entitiesConfig;
		uintPtr itemsArenaSize;
		uintPtr emitArenaSize;
		uint32 skinsCount;
		guiCreateConfig();
	};

	CAGE_API holder<guiClass> newGui(const guiCreateConfig &config);

	namespace detail
	{
		CAGE_API holder<pngImageClass> guiSkinTemplateExport(const skinConfigStruct &skin, uint32 resolution);
	}
}
