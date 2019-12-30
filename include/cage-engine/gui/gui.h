namespace cage
{
	struct CAGE_API GuiSkinConfig
	{
		GuiSkinElementLayout layouts[(uint32)GuiElementTypeEnum::TotalElements];
		GuiSkinWidgetDefaults defaults;
		uint32 textureName;
		GuiSkinConfig();
	};

	namespace privat
	{
		struct CAGE_API GuiComponents : public privat::GuiGeneralComponents, public privat::GuiWidgetsComponents, public privat::GuiLayoutsComponents
		{
			GuiComponents(EntityManager* ents);
		};
	}

	class CAGE_API Gui : private Immovable
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
		void setOutputSoundBus(MixingBus *bus);
		MixingBus *getOutputSoundBus() const;
		void setFocus(uint32 widget);
		uint32 getFocus() const;
		void controlUpdateStart(); // must be called before processing events
		void controlUpdateDone(); // accesses asset manager

		bool windowResize(const ivec2 &);
		bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseDouble(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseMove(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point);
		bool mouseWheel(sint32 wheel, ModifiersFlags modifiers, const ivec2 &point);
		bool keyPress(uint32 key, uint32 scanCode, ModifiersFlags modifiers);
		bool keyRepeat(uint32 key, uint32 scanCode, ModifiersFlags modifiers);
		bool keyRelease(uint32 key, uint32 scanCode, ModifiersFlags modifiers);
		bool keyChar(uint32 key);

		void handleWindowEvents(Window *window, sint32 order = 0);
		void skipAllEventsUntilNextUpdate();
		ivec2 getInputResolution() const;
		Delegate<bool(const ivec2&, vec2&)> eventCoordinatesTransformer; // called from controlUpdateStart or from any window events, it should return false to signal that the point is outside the gui, otherwise the point should be converted from window coordinate system to the gui output resolution coordinate system
		EventDispatcher<bool(uint32)> widgetEvent; // called from controlUpdateStart or window events

		GuiSkinConfig &skin(uint32 index = 0);
		const GuiSkinConfig &skin(uint32 index = 0) const;

		privat::GuiComponents &components();
		EntityManager *entities();
		AssetManager *assets();
	};

	struct CAGE_API GuiCreateConfig
	{
		AssetManager *assetMgr;
		EntityManagerCreateConfig *entitiesConfig;
		uintPtr itemsArenaSize;
		uintPtr emitArenaSize;
		uint32 skinsCount;
		GuiCreateConfig();
	};

	CAGE_API Holder<Gui> newGui(const GuiCreateConfig &config);

	namespace detail
	{
		CAGE_API Holder<Image> guiSkinTemplateExport(const GuiSkinConfig &skin, uint32 resolution);
	}
}
