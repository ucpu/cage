#ifndef guard_networkTcp_h_olkjhgv56ce
#define guard_networkTcp_h_olkjhgv56ce

#include "files.h"
#include "networkUtils.h"

namespace cage
{
	class CAGE_CORE_API TcpConnection : public File
	{
	public:
		String address() const; // remote address
		uint16 port() const; // remote port
	};

	CAGE_CORE_API Holder<TcpConnection> newTcpConnection(const String &address, uint16 port); // blocking

	class CAGE_CORE_API TcpServer : private Immovable
	{
	public:
		uint16 port() const; // local port

		// returns empty holder if no new peer has connected
		Holder<TcpConnection> accept(); // non-blocking
	};

	CAGE_CORE_API Holder<TcpServer> newTcpServer(uint16 port);
}

#endif // guard_networkTcp_h_olkjhgv56ce
