#ifndef guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2

#include "core.h"

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

	CAGE_ENGINE_API uint64 engineProfilingValues(EngineProfilingStatsFlags flags, EngineProfilingModeEnum mode);

	enum class EngineProfilingScopeEnum
	{
		Full,
		Short,
		Fps,
		None,
	};

	class CAGE_ENGINE_API EngineProfiling : private Immovable
	{
	public:
		vec2 screenPosition = vec2(1, 0);
		uint32 keyToggleProfilingScope = 290; // f1
		uint32 keyToggleProfilingMode = 291; // f2
		uint32 keyVisualizeBufferPrev = 292; // f3
		uint32 keyVisualizeBufferNext = 293; // f4
		uint32 keyToggleRenderMissingModels = 294; // f5
		uint32 keyToggleRenderSkeletonBones = 295; // f6
		ModifiersFlags keyModifiers = ModifiersFlags::Ctrl;
		EngineProfilingScopeEnum profilingScope = EngineProfilingScopeEnum::Full;
		EngineProfilingModeEnum profilingMode = EngineProfilingModeEnum::Maximum;
	};

	CAGE_ENGINE_API Holder<EngineProfiling> newEngineProfiling();
}

#endif // guard_engine_profiling_h_76C3A56EFED14909B79D2CEB55DDC4F2
