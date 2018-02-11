namespace cage
{
	namespace controlThread
	{
		CAGE_API extern eventDispatcher<bool()> initialize;
		CAGE_API extern eventDispatcher<bool()> finalize;
		CAGE_API extern eventDispatcher<bool()> update;
		CAGE_API extern eventDispatcher<bool()> assets;
		CAGE_API extern uint64 timePerTick;
		static const uint32 threadIndex = 0;
	}

	namespace graphicDispatchThread
	{
		CAGE_API extern eventDispatcher<bool()> initialize;
		CAGE_API extern eventDispatcher<bool()> finalize;
		CAGE_API extern eventDispatcher<bool()> render;
		CAGE_API extern eventDispatcher<bool()> swap;
		static const uint32 threadIndex = 1;
	}

	namespace graphicPrepareThread
	{
		CAGE_API extern eventDispatcher<bool()> initialize;
		CAGE_API extern eventDispatcher<bool()> finalize;
		CAGE_API extern eventDispatcher<bool()> prepare;
		CAGE_API extern stereoModeEnum stereoMode;
		static const uint32 threadIndex = 2;
	}

	namespace soundThread
	{
		CAGE_API extern eventDispatcher<bool()> initialize;
		CAGE_API extern eventDispatcher<bool()> finalize;
		CAGE_API extern eventDispatcher<bool()> sound;
		CAGE_API extern uint64 timePerTick;
		static const uint32 threadIndex = 3;
	}

	struct CAGE_API engineCreateConfig
	{
		uintPtr graphicEmitMemory;
		uintPtr graphicDispatchMemory;
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
		enum class profilingTimeFlags
		{
			None = 0,
			ControlTick = 1 << 0,
			ControlWait = 1 << 1,
			ControlEmit = 1 << 2,
			ControlSleep = 1 << 3,
			GraphicPrepareWait = 1 << 4,
			GraphicPrepareEmit = 1 << 5,
			GraphicPrepareTick = 1 << 6,
			GraphicDispatchWait = 1 << 7,
			GraphicDispatchTick = 1 << 8,
			GraphicDispatchSwap = 1 << 9,
			SoundEmit = 1 << 10,
			SoundTick = 1 << 11,
			SoundSleep = 1 << 12,
			FrameTime = profilingTimeFlags::GraphicDispatchWait | profilingTimeFlags::GraphicDispatchTick | profilingTimeFlags::GraphicDispatchSwap,
		};

		CAGE_API uint64 getTime(profilingTimeFlags flags, bool smooth);
	}
	GCHL_ENUM_BITS(engineProfiling::profilingTimeFlags);
}

#define ENGINE_GET_COMPONENT(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::CAGE_JOIN(T, Component)::component);
#define GUI_GET_COMPONENT(T,C,E) ::cage::CAGE_JOIN(T, Component) &C = (E)->value<::cage::CAGE_JOIN(T, Component)>(::cage::gui()->components().T);
