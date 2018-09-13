#ifndef guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2

namespace cage
{
	enum class engineProfilingStatsFlags
	{
		None = 0,
		ControlTick = 1 << 0,
		ControlEmit = 1 << 1,
		ControlSleep = 1 << 2,
		GraphicsPrepareWait = 1 << 3,
		GraphicsPrepareTick = 1 << 4,
		GraphicsDispatchWait = 1 << 5,
		GraphicsDispatchTick = 1 << 6,
		GraphicsDispatchSwap = 1 << 7,
		GraphicsDrawCalls = 1 << 8,
		GraphicsDrawPrimitives = 1 << 9,
		SoundWait = 1 << 10,
		SoundTick = 1 << 11,
		SoundSleep = 1 << 12,
		FrameTime = engineProfilingStatsFlags::GraphicsDispatchWait | engineProfilingStatsFlags::GraphicsDispatchTick | engineProfilingStatsFlags::GraphicsDispatchSwap,
	};
	GCHL_ENUM_BITS(engineProfilingStatsFlags);

	enum class engineProfilingModeEnum
	{
		Average,
		Maximum,
		Last,
	};

	CAGE_API uint64 engineProfilingValues(engineProfilingStatsFlags flags, engineProfilingModeEnum mode);

	enum class engineProfilingScopeEnum
	{
		Full,
		Short,
		Fps,
		None,
	};

	class CAGE_API engineProfilingClass
	{
	public:
		vec2 screenPosition;
		uint32 keyToggleProfilingScope;
		uint32 keyToggleProfilingMode;
		uint32 keyVisualizeBufferPrev;
		uint32 keyVisualizeBufferNext;
		uint32 keyToggleRenderMissingMeshes;
		uint32 keyToggleStereo;
		uint32 keyToggleFullscreen;
		modifiersFlags keyModifiers;
		engineProfilingScopeEnum profilingScope;
		engineProfilingModeEnum profilingMode;
	};

	CAGE_API holder<engineProfilingClass> newEngineProfiling();
}

#endif // guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
