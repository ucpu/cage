#ifndef header_guard_sound_private_h_mnb1v859ter8hz
#define header_guard_sound_private_h_mnb1v859ter8hz

#include <cage-engine/sound.h>

struct cubeb;

namespace cage
{
	class SoundContext;
	class MixingBus;
	struct SoundDataBuffer;

	namespace soundPrivat
	{
		void checkSoundIoError(int code);
		cubeb *soundioFromContext(SoundContext *context);

		struct BusInterface
		{
			const Delegate<void(MixingBus *)> busDestroyedDelegate;
			const Delegate<void(const SoundDataBuffer &)> busExecuteDelegate;

			BusInterface(Delegate<void(MixingBus *)> busDestroyedDelegate, Delegate<void(const SoundDataBuffer &)> busExecuteDelegate) :
				busDestroyedDelegate(busDestroyedDelegate), busExecuteDelegate(busExecuteDelegate)
			{}
		};

		void busAddInput(MixingBus *bus, const BusInterface *interface);
		void busRemoveInput(MixingBus *bus, const BusInterface *interface);
		void busAddOutput(MixingBus *bus, const BusInterface *interface);
		void busRemoveOutput(MixingBus *bus, const BusInterface *interface);
	}

	using namespace soundPrivat;
}

#endif
