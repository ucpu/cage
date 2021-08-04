#ifndef guard_engine_statistics_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_engine_statistics_h_76C3A56EFED14909B79D2CEB55DDC4F2

#include "core.h"

namespace cage
{
	enum class EngineStatisticsFlags
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
	GCHL_ENUM_BITS(EngineStatisticsFlags);

	enum class EngineStatisticsModeEnum
	{
		Average,
		Maximum,
		Last,
	};

	CAGE_ENGINE_API uint64 engineStatisticsValues(EngineStatisticsFlags flags, EngineStatisticsModeEnum mode);

	enum class EngineStatisticsScopeEnum
	{
		Full,
		Short,
		Fps,
		None,
	};

	class CAGE_ENGINE_API EngineStatistics : private Immovable
	{
	public:
		vec2 screenPosition = vec2(1, 0);
		uint32 keyToggleStatisticsScope = 290; // f1
		uint32 keyToggleStatisticsMode = 291; // f2
		uint32 keyToggleProfilingEnabled = 292; // f3
		uint32 keyVisualizeBufferPrev = 294; // f5
		uint32 keyVisualizeBufferNext = 295; // f6
		uint32 keyToggleRenderMissingModels = 296; // f7
		uint32 keyToggleRenderSkeletonBones = 297; // f8
		ModifiersFlags keyModifiers = ModifiersFlags::Ctrl;
		EngineStatisticsScopeEnum statisticsScope = EngineStatisticsScopeEnum::Full;
		EngineStatisticsModeEnum statisticsMode = EngineStatisticsModeEnum::Maximum;
	};

	CAGE_ENGINE_API Holder<EngineStatistics> newEngineStatistics();
}

#endif // guard_engine_statistics_h_76C3A56EFED14909B79D2CEB55DDC4F2
