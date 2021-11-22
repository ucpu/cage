#ifndef guard_statisticsGui_h_76C3A56EFED14909B79D2CEB55DDC4F2
#define guard_statisticsGui_h_76C3A56EFED14909B79D2CEB55DDC4F2

#include <cage-engine/core.h>

namespace cage
{
	enum class StatisticsGuiFlags
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
	GCHL_ENUM_BITS(StatisticsGuiFlags);

	enum class StatisticsGuiModeEnum
	{
		Average,
		Maximum,
		Last,
	};

	uint64 engineStatisticsValues(StatisticsGuiFlags flags, StatisticsGuiModeEnum mode);

	enum class StatisticsGuiScopeEnum
	{
		Full,
		Short,
		Fps,
		None,
	};

	class StatisticsGui : private Immovable
	{
	public:
		Vec2 screenPosition = Vec2(1, 0);
		uint32 keyToggleStatisticsScope = 290; // f1
		uint32 keyToggleStatisticsMode = 291; // f2
		uint32 keyToggleProfilingEnabled = 292; // f3
		uint32 keyVisualizeBufferPrev = 294; // f5
		uint32 keyVisualizeBufferNext = 295; // f6
		uint32 keyToggleRenderMissingModels = 296; // f7
		uint32 keyToggleRenderSkeletonBones = 297; // f8
		ModifiersFlags keyModifiers = ModifiersFlags::Ctrl;
		StatisticsGuiScopeEnum statisticsScope = StatisticsGuiScopeEnum::Full;
		StatisticsGuiModeEnum statisticsMode = StatisticsGuiModeEnum::Maximum;
	};

	Holder<StatisticsGui> newStatisticsGui();
}

#endif // guard_statisticsGui_h_76C3A56EFED14909B79D2CEB55DDC4F2
