#include <soundio/soundio.h>

namespace cage
{
	class SoundContext;
	class MixingBus;
	struct SoundDataBuffer;

	namespace soundPrivat
	{
		void checkSoundIoError(int code);
		SoundIo *soundioFromContext(SoundContext *context);
		MemoryArena linksArenaFromContext(SoundContext *context);

		struct busInterfaceStruct
		{
			const Delegate<void(MixingBus*)> busDestroyedDelegate;
			const Delegate<void(const SoundDataBuffer&)> busExecuteDelegate;
			busInterfaceStruct(Delegate<void(MixingBus*)> busDestroyedDelegate, Delegate<void(const SoundDataBuffer&)> busExecuteDelegate) :
				busDestroyedDelegate(busDestroyedDelegate), busExecuteDelegate(busExecuteDelegate) {}
		};

		void busAddInput(MixingBus *bus, const busInterfaceStruct *interface);
		void busRemoveInput(MixingBus *bus, const busInterfaceStruct *interface);
		void busAddOutput(MixingBus *bus, const busInterfaceStruct *interface);
		void busRemoveOutput(MixingBus *bus, const busInterfaceStruct *interface);
	}

	using namespace soundPrivat;
}
