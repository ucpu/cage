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
		memoryArena linksArenaFromContext(soundContext *context);

		struct busInterfaceStruct
		{
			const delegate<void(mixingBus*)> busDestroyedDelegate;
			const delegate<void(const soundDataBufferStruct&)> busExecuteDelegate;
			busInterfaceStruct(delegate<void(mixingBus*)> busDestroyedDelegate, delegate<void(const soundDataBufferStruct&)> busExecuteDelegate) :
				busDestroyedDelegate(busDestroyedDelegate), busExecuteDelegate(busExecuteDelegate) {}
		};

		void busAddInput(mixingBus *bus, const busInterfaceStruct *interface);
		void busRemoveInput(mixingBus *bus, const busInterfaceStruct *interface);
		void busAddOutput(mixingBus *bus, const busInterfaceStruct *interface);
		void busRemoveOutput(mixingBus *bus, const busInterfaceStruct *interface);
	}

	using namespace soundPrivat;
}
