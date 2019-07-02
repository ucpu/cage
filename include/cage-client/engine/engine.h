namespace cage
{
	struct CAGE_API controlThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> update;
		eventDispatcher<bool()> assets;
		uint64 timePerTick;
		static const uint32 threadIndex = 0;
		controlThreadClass();
	};
	CAGE_API controlThreadClass &controlThread();

	struct CAGE_API graphicsDispatchThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> render;
		eventDispatcher<bool()> swap;
		static const uint32 threadIndex = 1;
		graphicsDispatchThreadClass();
	};
	CAGE_API graphicsDispatchThreadClass &graphicsDispatchThread();

	struct CAGE_API graphicsPrepareThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> prepare;
		stereoModeEnum stereoMode;
		static const uint32 threadIndex = 3;
		graphicsPrepareThreadClass();
	};
	CAGE_API graphicsPrepareThreadClass &graphicsPrepareThread();

	struct CAGE_API soundThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> sound;
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
		entityManagerCreateConfig *entities;
		assetManagerCreateConfig *assets;
		guiCreateConfig *gui;
		soundContextCreateConfig *soundContext;
		speakerCreateConfig *speaker;
		engineCreateConfig();
	};

	CAGE_API void engineInitialize(const engineCreateConfig &config);
	CAGE_API void engineStart();
	CAGE_API void engineStop();
	CAGE_API void engineFinalize();

	CAGE_API soundContextClass *sound();
	CAGE_API assetManager *assets();
	CAGE_API entityManager *entities();
	CAGE_API windowClass *window();
	CAGE_API guiClass *gui();
	CAGE_API speakerClass *speaker();
	CAGE_API busClass *masterMixer();
	CAGE_API busClass *musicMixer();
	CAGE_API busClass *effectsMixer();
	CAGE_API busClass *guiMixer();
	CAGE_API uint64 currentControlTime();
}

#define ENGINE_GET_COMPONENT(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::CAGE_JOIN(T, Component)::component);
#define GUI_GET_COMPONENT(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::gui()->components().T);
