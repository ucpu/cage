#include <soundio/soundio.h>

namespace cage
{
	class soundContext;
	class mixingBus;
	struct soundDataBufferStruct;

	namespace soundPrivat
	{
		void checkSoundIoError(int code);
		SoundIo *soundioFromContext(soundContext *context);
		MemoryArena linksArenaFromContext(soundContext *context);

		struct busInterfaceStruct
		{
			const Delegate<void(mixingBus*)> busDestroyedDelegate;
			const Delegate<void(const soundDataBufferStruct&)> busExecuteDelegate;
			busInterfaceStruct(Delegate<void(mixingBus*)> busDestroyedDelegate, Delegate<void(const soundDataBufferStruct&)> busExecuteDelegate) :
				busDestroyedDelegate(busDestroyedDelegate), busExecuteDelegate(busExecuteDelegate) {}
		};

		void busAddInput(mixingBus *bus, const busInterfaceStruct *interface);
		void busRemoveInput(mixingBus *bus, const busInterfaceStruct *interface);
		void busAddOutput(mixingBus *bus, const busInterfaceStruct *interface);
		void busRemoveOutput(mixingBus *bus, const busInterfaceStruct *interface);
	}

	using namespace soundPrivat;
}
