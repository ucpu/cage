#ifndef guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_
#define guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/network.h>
#include <cage-core/log.h>
#include <cage-core/memoryBuffer.h>

#ifdef CAGE_SYSTEM_WINDOWS

#include "../incWin.h"
#include <winsock2.h>         // For socket(), connect(), send(), and recv()
#include <ws2tcpip.h>
typedef char raw_type;       // Type used for raw data on this platform
#undef near
#undef far

#else

#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <sys/ioctl.h>       // For ioctl()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in
#include <errno.h>           // For errno
typedef void raw_type;       // Type used for raw data on this platform
typedef int SOCKET;
#define WSAGetLastError() errno
#define closesocket close
#define ioctlsocket ioctl
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAECONNRESET ECONNRESET
#define INVALID_SOCKET -1

#endif

namespace cage
{
	namespace privat
	{
		struct addr
		{
			addr();
			void translate(string &address, uint16 &port, bool domain = false) const;

			bool operator < (const addr &other) const // fast (binary) comparison
			{
				return detail::memcmp(&storage, &other.storage, sizeof(storage)) < 0;
			}

			bool operator == (const addr &other) const // fast (binary) comparison
			{
				return detail::memcmp(&storage, &other.storage, sizeof(storage)) == 0;
			}

		private:
			sockaddr_storage storage;
			socklen_t addrlen;

			friend struct sock;
			friend struct addrList;
		};

		struct sock
		{
			sock(); // invalid socket
			sock(int family, int type, int protocol); // create new socket
			sock(int family, int type, int protocol, SOCKET descriptor, bool connected); // copy socket
			sock(sock &&other) noexcept;
			void operator = (sock &&other) noexcept;
			~sock();

			sock(const sock &) = delete;
			void operator = (const sock &other) = delete;

			void setBlocking(bool blocking);
			void setReuseaddr(bool reuse);
			void setBroadcast(bool broadcast);
			void setBufferSize(uint32 sending, uint32 receiving);
			void setBufferSize(uint32 size);

			void bind(const addr &localAddress);
			void connect(const addr &remoteAddress);
			void listen(int backlog = 5);
			sock accept();

			void close();
			bool isValid() const;
			addr getLocalAddress() const;
			addr getRemoteAddress() const;

			uintPtr available() const;
			void send(const void *buffer, uintPtr bufferSize);
			void sendTo(const void *buffer, uintPtr bufferSize, const addr &remoteAddress);
			uintPtr recv(void *buffer, uintPtr bufferSize, int flags = 0);
			uintPtr recvFrom(void *buffer, uintPtr bufferSize, addr &remoteAddress, int flags = 0);

			bool operator < (const sock &other) const // fast comparison
			{
				return descriptor < other.descriptor;
			}

			int getFamily() const { return family; };
			int getType() const { return type; };
			int getProtocol() const { return protocol; };
			bool getConnected() const { return connected; }

		private:
			SOCKET descriptor;
			int family, type, protocol;
			bool connected;
		};

		struct addrList
		{
			addrList(const string &address, uint16 port, int family, int type, int protocol, int flags);
			addrList(const char *address, uint16 port, int family, int type, int protocol, int flags);
			~addrList();

			bool valid() const;
			addr address() const;
			int family() const;
			int type() const;
			int protocol() const;
			void getAll(addr &address, int &family, int &type, int &protocol) const;
			void next();

		private:
			addrinfo *start, *current;
		};
	}
}

#endif // guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_
