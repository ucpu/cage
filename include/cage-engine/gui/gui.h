namespace cage
{
	struct CAGE_API guiSkinConfig
	{
		guiSkinElementLayout layouts[(uint32)elementTypeEnum::TotalElements];
		guiSkinWidgetDefaults defaults;
		uint32 textureName;
		guiSkinConfig();
	};

	struct CAGE_API guiComponents : public privat::guiGeneralComponents, public privat::guiWidgetsComponents, public privat::guiLayoutsComponents
	{
		guiComponents(entityManager *ents);
	};

	class CAGE_API guiManager : private immovable
	{
	public:
		void graphicsInitialize();
		void graphicsFinalize();
		void graphicsRender();

		//void soundInitialize();
		//void soundFinalize();
		//void soundRender();

		void setOutputResolution(const ivec2 &resolution, real retina = 1); // resolution: pixels; retina: how many pixels per point (1D)
		void setZoom(real zoom); // pixels per point (1D)
		ivec2 getOutputResolution() const;
		real getOutputRetina() const;
		real getZoom() const;
		void setOutputSoundBus(mixingBus *bus);
		mixingBus *getOutputSoundBus() const;
		void setFocus(uint32 widget);
		uint32 getFocus() const;
		void controlUpdateStart(); // must be called before processing events
		void controlUpdateDone(); // accesses asset manager

		bool windowResize(const ivec2 &);
		bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point);
		bool mouseDouble(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point);
		bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point);
		bool mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, const ivec2 &point);
		bool mouseWheel(sint32 wheel, modifiersFlags modifiers, const ivec2 &point);
		bool keyPress(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyRepeat(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyRelease(uint32 key, uint32 scanCode, modifiersFlags modifiers);
		bool keyChar(uint32 key);

		void handleWindowEvents(windowHandle *window, sint32 order = 0);
		void skipAllEventsUntilNextUpdate();
		ivec2 getInputResolution() const;
		delegate<bool(const ivec2&, vec2&)> eventCoordinatesTransformer; // called from controlUpdateStart or from any window events, it should return false to signal that the point is outside the gui, otherwise the point should be converted from window coordinate system to the gui output resolution coordinate system
		eventDispatcher<bool(uint32)> widgetEvent; // called from controlUpdateStart or window events

		guiSkinConfig &skin(uint32 index = 0);
		const guiSkinConfig &skin(uint32 index = 0) const;

		guiComponents &components();
		entityManager *entities();
		assetManager *assets();
	};

	struct CAGE_API guiManagerCreateConfig
	{
		assetManager *assetMgr;
		entityManagerCreateConfig *entitiesConfig;
		uintPtr itemsArenaSize;
		uintPtr emitArenaSize;
		uint32 skinsCount;
		guiManagerCreateConfig();
	};

	CAGE_API holder<guiManager> newGuiManager(const guiManagerCreateConfig &config);

	namespace detail
	{
		CAGE_API holder<image> guiSkinTemplateExport(const guiSkinConfig &skin, uint32 resolution);
	}
}
