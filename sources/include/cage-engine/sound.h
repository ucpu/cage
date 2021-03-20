#ifndef guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
#define guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC

#include "soundCommon.h"

namespace cage
{
	class CAGE_ENGINE_API Sound : private Immovable
	{
	public:
		bool loopBeforeStart = false;
		bool loopAfterEnd = false;

		void initialize(Holder<Audio> &&audio);

		void process(const SoundCallbackData &data);
	};

	CAGE_ENGINE_API Holder<Sound> newSound();

	CAGE_ENGINE_API AssetScheme genAssetSchemeSound(uint32 threadIndex);
	constexpr uint32 AssetSchemeIndexSound = 20;
}

#endif // guard_sound_h_8EF0985E04CD4714B530EA7D605E92EC
