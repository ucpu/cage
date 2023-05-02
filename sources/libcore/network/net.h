#ifndef guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_
#define guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_

#include <cage-core/networkUtils.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/debug.h>

#ifdef CAGE_SYSTEM_WINDOWS

#include "../incWin.h"
#include <winsock2.h>
#include <ws2tcpip.h>
typedef char raw_type;
#define POLLRDHUP 0
#undef POLLPRI // WSAPoll rejects POLLPRI with an error
#define POLLPRI 0
#undef near
#undef far

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
typedef void raw_type;
typedef int SOCKET;
#define WSAGetLastError() errno
#define closesocket close
#define ioctlsocket ioctl
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAECONNRESET ECONNRESET
#define INVALID_SOCKET -1
#define WSAPoll poll

#endif

namespace cage
{
	namespace privat
	{
		struct Addr
		{
			Addr();

			void translate(String &address, uint16 &port, bool domain = false) const;

			bool operator < (const Addr &other) const // fast (binary) comparison
			{
				return detail::memcmp(&storage, &other.storage, sizeof(storage)) < 0;
			}

			bool operator == (const Addr &other) const // fast (binary) comparison
			{
				return detail::memcmp(&storage, &other.storage, sizeof(storage)) == 0;
			}

		private:
			sockaddr_storage storage = {};
			socklen_t addrlen = 0;

			friend struct Sock;
			friend struct AddrList;
		};

		struct Sock : private Noncopyable
		{
			Sock(); // invalid socket
			Sock(int family, int type, int protocol); // create new socket
			Sock(int family, int type, int protocol, SOCKET descriptor, bool connected); // copy socket
			Sock(Sock &&other) noexcept;
			void operator = (Sock &&other) noexcept;
			~Sock();

			Sock(const Sock &) = delete;
			void operator = (const Sock &other) = delete;

			void setBlocking(bool blocking);
			void setReuseaddr(bool reuse);
			void setBroadcast(bool broadcast);
			void setBufferSize(uint32 sending, uint32 receiving);
			void setBufferSize(uint32 size);

			void bind(const Addr &localAddress);
			void connect(const Addr &remoteAddress);
			void listen(int backlog = 5);
			Sock accept();

			void close();
			bool isValid() const;
			Addr getLocalAddress() const;
			Addr getRemoteAddress() const;

			uintPtr available() const;
			uint16 events() const;
			void send(const void *buffer, uintPtr bufferSize);
			void sendTo(const void *buffer, uintPtr bufferSize, const Addr &remoteAddress);
			uintPtr recv(void *buffer, uintPtr bufferSize, int flags = 0);
			uintPtr recvFrom(void *buffer, uintPtr bufferSize, Addr &remoteAddress, int flags = 0);

			bool operator < (const Sock &other) const // fast comparison
			{
				return descriptor < other.descriptor;
			}

			int getFamily() const { return family; };
			int getType() const { return type; };
			int getProtocol() const { return protocol; };
			bool getConnected() const { return connected; }

		private:
			SOCKET descriptor = INVALID_SOCKET;
			int family = -1, type = -1, protocol = -1;
			bool connected = false;
		};

		struct AddrList : private Immovable
		{
			AddrList(const String &address, uint16 port, int family, int type, int protocol, int flags);
			AddrList(const char *address, uint16 port, int family, int type, int protocol, int flags);
			~AddrList();

			bool valid() const;
			Addr address() const;
			int family() const;
			int type() const;
			int protocol() const;
			void getAll(Addr &address, int &family, int &type, int &protocol) const;
			void next();

		private:
			addrinfo *start = nullptr, *current = nullptr;
		};
	}
}

#endif // guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_
