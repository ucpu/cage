#include <soundio/soundio.h>

namespace cage
{
	class soundContextClass;
	class busClass;
	struct soundDataBufferStruct;

	namespace soundPrivat
	{
		void checkSoundIoError(int code);
		SoundIo *soundioFromContext(soundContextClass *context);
		memoryArena linksArenaFromContext(soundContextClass *context);

		struct busInterfaceStruct
		{
			const delegate<void(busClass*)> busDestroyedDelegate;
			const delegate<void(const soundDataBufferStruct&)> busExecuteDelegate;
			busInterfaceStruct(delegate<void(busClass*)> busDestroyedDelegate, delegate<void(const soundDataBufferStruct&)> busExecuteDelegate) :
				busDestroyedDelegate(busDestroyedDelegate), busExecuteDelegate(busExecuteDelegate) {}
		};

		void busAddInput(busClass *bus, const busInterfaceStruct *interface);
		void busRemoveInput(busClass *bus, const busInterfaceStruct *interface);
		void busAddOutput(busClass *bus, const busInterfaceStruct *interface);
		void busRemoveOutput(busClass *bus, const busInterfaceStruct *interface);
	}

	using namespace soundPrivat;
}
