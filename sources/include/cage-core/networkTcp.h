#ifndef guard_networkTcp_h_olkjhgv56ce
#define guard_networkTcp_h_olkjhgv56ce

#include <cage-core/files.h>
#include <cage-core/networkUtils.h>

namespace cage
{
	struct CAGE_CORE_API TcpRemoteInfo
	{
		String address;
		uint16 port = 0;
	};

	class CAGE_CORE_API TcpConnection : public File
	{
	public:
		TcpRemoteInfo remoteInfo() const;
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
