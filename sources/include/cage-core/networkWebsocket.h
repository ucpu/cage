#ifndef guard_networkWebsocket_h_lkjhgse897drzh4
#define guard_networkWebsocket_h_lkjhgse897drzh4

#include <cage-core/files.h>
#include <cage-core/networkUtils.h>

namespace cage
{
	// websocket framing is preserved
	// full sequence of frames (up to first FIN) will be read before more data are made available
	class CAGE_CORE_API WebsocketConnection : public File
	{
	public:
		String address() const; // remote address
		uint16 port() const; // remote port
	};

	CAGE_CORE_API Holder<WebsocketConnection> newWebsocketConnection(const String &address, uint16 port); // blocking

	class CAGE_CORE_API WebsocketServer : private Immovable
	{
	public:
		uint16 port() const; // local port

		// returns empty holder if no new peer has connected
		Holder<WebsocketConnection> accept(); // non-blocking when no new connection, but blocking when handshaking
	};

	CAGE_CORE_API Holder<WebsocketServer> newWebsocketServer(uint16 port);
}

#endif // guard_networkWebsocket_h_lkjhgse897drzh4
