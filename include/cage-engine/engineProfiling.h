#ifndef guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2

namespace cage
{
	enum class EngineProfilingStatsFlags
	{
		None = 0,
		Control = 1 << 0,
		Sound = 1 << 1,
		GraphicsPrepare = 1 << 2,
		GraphicsDispatch = 1 << 3,
		FrameTime = 1 << 4,
		DrawCalls = 1 << 5,
		DrawPrimitives = 1 << 6,
		Entities = 1 << 7,
	};

	enum class EngineProfilingModeEnum
	{
		Average,
		Maximum,
		Last,
	};

	CAGE_API uint64 engineProfilingValues(EngineProfilingStatsFlags flags, EngineProfilingModeEnum mode);

	enum class EngineProfilingScopeEnum
	{
		Full,
		Short,
		Fps,
		None,
	};

	class CAGE_API EngineProfiling : private Immovable
	{
	public:
		vec2 screenPosition;
		uint32 keyToggleProfilingScope;
		uint32 keyToggleProfilingMode;
		uint32 keyVisualizeBufferPrev;
		uint32 keyVisualizeBufferNext;
		uint32 keyToggleRenderMissingMeshes;
		uint32 keyToggleRenderSkeletonBones;
		uint32 keyToggleStereo;
		ModifiersFlags keyModifiers;
		EngineProfilingScopeEnum profilingScope;
		EngineProfilingModeEnum profilingMode;
		EngineProfiling();
	};

	CAGE_API Holder<EngineProfiling> newEngineProfiling();
}

#endif // guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
