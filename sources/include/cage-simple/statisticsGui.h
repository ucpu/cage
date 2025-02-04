#ifndef guard_statisticsGui_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_statisticsGui_h_76C3A56EFED14909B79D2CEB55DDC4F2

#include <cage-engine/core.h>

namespace cage
{
	enum class StatisticsGuiFlags : uint32
	{
		None = 0,
		CpuUtilization = 1 << 0, // % main thread cpu use
		DynamicResolution = 1 << 1,
		ControlThreadTime = 1 << 2,
		SoundThreadTime = 1 << 3,
		PrepareThreadTime = 1 << 4,
		GpuTime = 1 << 5,
		FrameTime = 1 << 6,
		DrawCalls = 1 << 7,
		DrawPrimitives = 1 << 8,
		Entities = 1 << 9,
	};
	GCHL_ENUM_BITS(StatisticsGuiFlags);

	enum class StatisticsGuiModeEnum : uint8
	{
		Average,
		Maximum,
		Latest,
	};

	uint64 engineStatisticsValues(StatisticsGuiFlags flags, StatisticsGuiModeEnum mode);

	enum class StatisticsGuiScopeEnum : uint8
	{
		None,
		Fps,
		Full,
	};

	class StatisticsGui : private Immovable
	{
	public:
		Vec2 screenPosition = Vec2(1, 0);
		uint32 keyToggleStatisticsScope = 290; // f1
		uint32 keyToggleStatisticsMode = 291; // f2
		uint32 keyToggleProfilingEnabled = 292; // f3
		uint32 keyToggleRenderMissingModels = 294; // f5
		uint32 keyToggleRenderSkeletonBones = 295; // f6
		uint32 keyDecreaseGamma = 298; // f9
		uint32 keyIncreaseGamma = 299; // f10
		ModifiersFlags keyModifiers = ModifiersFlags::Ctrl;
		StatisticsGuiScopeEnum statisticsScope = StatisticsGuiScopeEnum::Full;
		StatisticsGuiModeEnum statisticsMode = StatisticsGuiModeEnum::Maximum;
	};

	Holder<StatisticsGui> newStatisticsGui();
}

#endif // guard_statisticsGui_h_76C3A56EFED14909B79D2CEB55DDC4F2
