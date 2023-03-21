#ifndef guard_networkDiscovery_h_ds5gh4q6as5d48
#define guard_networkDiscovery_h_ds5gh4q6as5d48

#include "networkUtils.h"

namespace cage
{
	struct CAGE_CORE_API DiscoveryPeer
	{
		String message;
		String address;
		String domain;
		uint16 port = 0;
	};

	class CAGE_CORE_API DiscoveryClient : private Immovable
	{
	public:
		void update();
		void addServer(const String &address, uint16 listenPort);
		Holder<PointerRange<DiscoveryPeer>> peers() const;
	};

	CAGE_CORE_API Holder<DiscoveryClient> newDiscoveryClient(uint16 listenPort, uint32 gameId);

	class CAGE_CORE_API DiscoveryServer : private Immovable
	{
	public:
		String message;

		void update();
	};

	CAGE_CORE_API Holder<DiscoveryServer> newDiscoveryServer(uint16 listenPort, uint16 gamePort, uint32 gameId);
}

#endif // guard_networkDiscovery_h_ds5gh4q6as5d48
