namespace cage
{
	CAGE_API struct CAGE_API controlThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> update;
		eventDispatcher<bool()> assets;
		uint64 timePerTick;
		static const uint32 threadIndex = 0;
		controlThreadClass();
	} &controlThread();

	CAGE_API struct CAGE_API graphicsDispatchThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> render;
		eventDispatcher<bool()> swap;
		static const uint32 threadIndex = 1;
		graphicsDispatchThreadClass();
	} &graphicsDispatchThread();

	CAGE_API struct CAGE_API graphicsPrepareThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> prepare;
		stereoModeEnum stereoMode;
		static const uint32 threadIndex = 2;
		graphicsPrepareThreadClass();
	} &graphicsPrepareThread();

	CAGE_API struct CAGE_API soundThreadClass
	{
		eventDispatcher<bool()> initialize;
		eventDispatcher<bool()> finalize;
		eventDispatcher<bool()> sound;
		uint64 timePerTick;
		static const uint32 threadIndex = 3;
		soundThreadClass();
	} &soundThread();

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
	CAGE_API assetManagerClass *assets();
	CAGE_API entityManagerClass *entities();
	CAGE_API windowClass *window();
	CAGE_API guiClass *gui();
	CAGE_API speakerClass *speaker();
	CAGE_API busClass *masterMixer();
	CAGE_API busClass *musicMixer();
	CAGE_API busClass *effectsMixer();
	CAGE_API busClass *guiMixer();
	CAGE_API uint64 currentControlTime();

	namespace engineProfiling
	{
		enum class profilingFlags
		{
			None = 0,
			ControlTick = 1 << 0,
			ControlWait = 1 << 1,
			ControlEmit = 1 << 2,
			ControlSleep = 1 << 3,
			GraphicsPrepareWait = 1 << 4,
			GraphicsPrepareEmit = 1 << 5,
			GraphicsPrepareTick = 1 << 6,
			GraphicsDispatchWait = 1 << 7,
			GraphicsDispatchTick = 1 << 8,
			GraphicsDispatchSwap = 1 << 9,
			GraphicsDrawCalls = 1 << 10,
			GraphicsDrawPrimitives = 1 << 11,
			SoundEmit = 1 << 12,
			SoundTick = 1 << 13,
			SoundSleep = 1 << 14,
			FrameTime = profilingFlags::GraphicsDispatchWait | profilingFlags::GraphicsDispatchTick | profilingFlags::GraphicsDispatchSwap,
		};

		CAGE_API uint64 getProfilingValue(profilingFlags flags, bool smooth);
	}
	GCHL_ENUM_BITS(engineProfiling::profilingFlags);
}

#define ENGINE_GET_COMPONENT(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::CAGE_JOIN(T, Component)::component);
#define GUI_GET_COMPONENT(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::gui()->components().T);
