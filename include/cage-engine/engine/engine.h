namespace cage
{
	struct CAGE_API EngineControlThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> update;
		EventDispatcher<bool()> assets;
		uint64 timePerTick;
		static const uint32 threadIndex = 0;
		EngineControlThread();
	};
	CAGE_API EngineControlThread &controlThread();

	struct CAGE_API EngineGraphicsDispatchThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> render;
		EventDispatcher<bool()> swap;
		static const uint32 threadIndex = 1;
		EngineGraphicsDispatchThread();
	};
	CAGE_API EngineGraphicsDispatchThread &graphicsDispatchThread();

	struct CAGE_API EngineGraphicsPrepareThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> prepare;
		StereoModeEnum stereoMode;
		static const uint32 threadIndex = 3;
		EngineGraphicsPrepareThread();
	};
	CAGE_API EngineGraphicsPrepareThread &graphicsPrepareThread();

	struct CAGE_API EngineSoundThread
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> sound;
		uint64 timePerTick;
		static const uint32 threadIndex = 4;
		EngineSoundThread();
	};
	CAGE_API EngineSoundThread &soundThread();

	struct CAGE_API EngineCreateConfig
	{
		uintPtr graphicsEmitMemory;
		uintPtr graphicsDispatchMemory;
		uintPtr soundEmitMemory;
		EntityManagerCreateConfig *entities;
		AssetManagerCreateConfig *assets;
		GuiCreateConfig *gui;
		SoundContextCreateConfig *soundContext;
		SpeakerCreateConfig *speaker;
		EngineCreateConfig();
	};

	CAGE_API void engineInitialize(const EngineCreateConfig &config);
	CAGE_API void engineStart();
	CAGE_API void engineStop();
	CAGE_API void engineFinalize();

	CAGE_API SoundContext *engineSound();
	CAGE_API AssetManager *engineAssets();
	CAGE_API EntityManager *engineEntities();
	CAGE_API Window *engineWindow();
	CAGE_API Gui *engineGui();
	CAGE_API Speaker *engineSpeaker();
	CAGE_API MixingBus *engineMasterMixer();
	CAGE_API MixingBus *engineMusicMixer();
	CAGE_API MixingBus *engineEffectsMixer();
	CAGE_API MixingBus *engineGuiMixer();
	CAGE_API uint64 engineControlTime();
}

#define CAGE_COMPONENT_ENGINE(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::CAGE_JOIN(T, Component)::component);
#define CAGE_COMPONENT_GUI(T,C,E) ::cage::CAGE_JOIN(Gui, CAGE_JOIN(T, Component)) &C = (E)->value<::cage::CAGE_JOIN(Gui, CAGE_JOIN(T, Component))>(::cage::engineGui()->components().T);
