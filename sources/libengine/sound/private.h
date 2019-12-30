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

		struct BusInterface
		{
			const Delegate<void(MixingBus*)> busDestroyedDelegate;
			const Delegate<void(const SoundDataBuffer&)> busExecuteDelegate;
			BusInterface(Delegate<void(MixingBus*)> busDestroyedDelegate, Delegate<void(const SoundDataBuffer&)> busExecuteDelegate) :
				busDestroyedDelegate(busDestroyedDelegate), busExecuteDelegate(busExecuteDelegate) {}
		};

		void busAddInput(MixingBus *bus, const BusInterface *interface);
		void busRemoveInput(MixingBus *bus, const BusInterface *interface);
		void busAddOutput(MixingBus *bus, const BusInterface *interface);
		void busRemoveOutput(MixingBus *bus, const BusInterface *interface);
	}

	using namespace soundPrivat;
}
