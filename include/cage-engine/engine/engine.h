namespace cage
{
	struct CAGE_API controlThreadClass
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> update;
		EventDispatcher<bool()> assets;
		uint64 timePerTick;
		static const uint32 threadIndex = 0;
		controlThreadClass();
	};
	CAGE_API controlThreadClass &controlThread();

	struct CAGE_API graphicsDispatchThreadClass
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> render;
		EventDispatcher<bool()> swap;
		static const uint32 threadIndex = 1;
		graphicsDispatchThreadClass();
	};
	CAGE_API graphicsDispatchThreadClass &graphicsDispatchThread();

	struct CAGE_API graphicsPrepareThreadClass
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> prepare;
		StereoModeEnum stereoMode;
		static const uint32 threadIndex = 3;
		graphicsPrepareThreadClass();
	};
	CAGE_API graphicsPrepareThreadClass &graphicsPrepareThread();

	struct CAGE_API soundThreadClass
	{
		EventDispatcher<bool()> initialize;
		EventDispatcher<bool()> finalize;
		EventDispatcher<bool()> sound;
		uint64 timePerTick;
		static const uint32 threadIndex = 4;
		soundThreadClass();
	};
	CAGE_API soundThreadClass &soundThread();

	struct CAGE_API engineCreateConfig
	{
		uintPtr graphicsEmitMemory;
		uintPtr graphicsDispatchMemory;
		uintPtr soundEmitMemory;
		EntityManagerCreateConfig *entities;
		AssetManagerCreateConfig *assets;
		guiManagerCreateConfig *gui;
		soundContextCreateConfig *soundContext;
		speakerOutputCreateConfig *speaker;
		engineCreateConfig();
	};

	CAGE_API void engineInitialize(const engineCreateConfig &config);
	CAGE_API void engineStart();
	CAGE_API void engineStop();
	CAGE_API void engineFinalize();

	CAGE_API soundContext *sound();
	CAGE_API AssetManager *assets();
	CAGE_API EntityManager *entities();
	CAGE_API windowHandle *window();
	CAGE_API guiManager *gui();
	CAGE_API speakerOutput *speaker();
	CAGE_API mixingBus *masterMixer();
	CAGE_API mixingBus *musicMixer();
	CAGE_API mixingBus *effectsMixer();
	CAGE_API mixingBus *guiMixer();
	CAGE_API uint64 currentControlTime();
}

#define CAGE_COMPONENT_ENGINE(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::CAGE_JOIN(T, Component)::component);
#define CAGE_COMPONENT_GUI(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::gui()->components().T);
