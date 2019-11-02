#ifndef guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2

namespace cage
{
	enum class engineProfilingStatsFlags
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

	class CAGE_API engineProfiling : private immovable
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
		modifiersFlags keyModifiers;
		engineProfilingScopeEnum profilingScope;
		engineProfilingModeEnum profilingMode;
		engineProfiling();
	};

	CAGE_API holder<engineProfiling> newEngineProfiling();
}

#endif // guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
