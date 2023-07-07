#ifndef guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_
#define guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_

#include <array>
#include <cage-core/debug.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/networkUtils.h>

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

	#include <arpa/inet.h>
	#include <errno.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <poll.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <unistd.h>
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
	struct Serializer;
	struct Deserializer;

	namespace privat
	{
		constexpr uint32 CageMagic = 0x65676163;

#ifdef CAGE_SYSTEM_WINDOWS
		constexpr const char *EmptyAddress = "";
#else
		constexpr const char *EmptyAddress = nullptr;
#endif // CAGE_SYSTEM_WINDOWS

		struct Sock;
		struct AddrList;

		struct Addr
		{
			Addr() = default;
			Addr(std::array<uint8, 4> ipv4, uint16 port); // arguments are in host byte order
			Addr(std::array<uint8, 16> ipv6, uint16 port); // arguments are in host byte order

			std::pair<String, uint16> translate(bool domain) const;

			bool operator<(const Addr &other) const // fast (binary) comparison
			{
				if (addrlen == other.addrlen)
					return detail::memcmp(&storage, &other.storage, addrlen) < 0;
				return addrlen < other.addrlen;
			}

			bool operator==(const Addr &other) const // fast (binary) comparison
			{
				return addrlen == other.addrlen && detail::memcmp(&storage, &other.storage, addrlen) == 0;
			}

			int getFamily() const { return storage.ss_family; }

			std::array<uint8, 4> getIp4() const; // host byte order
			std::array<uint8, 16> getIp6() const; // host byte order
			uint16 getPort() const; // host byte order

		private:
			sockaddr_storage storage = {}; // port and ip are in network byte order
			socklen_t addrlen = 0;

			friend Sock;
			friend AddrList;
			friend Serializer &operator<<(Serializer &s, const Addr &v);
			friend Deserializer &operator>>(Deserializer &s, Addr &v);
		};

		struct AddrList : private Immovable
		{
			// flags = AI_PASSIVE -> bind/listen
			// flags = 0 -> connect/sendTo

			AddrList(const String &address, uint16 port, int family, int type, int protocol, int flags); // port is in host byte order
			AddrList(const char *address, uint16 port, int family, int type, int protocol, int flags); // port is in host byte order
			~AddrList();

			bool valid() const;
			void next();

			Sock sock() const; // uses family, type, and protocol; it is up to you to use the address
			Addr address() const;
			int family() const;
			int type() const;
			int protocol() const;

		private:
			addrinfo *start = nullptr, *current = nullptr;
		};

		struct Sock : private Noncopyable
		{
			Sock(); // invalid socket
			Sock(int family, int type, int protocol); // create new socket
			Sock(int family, int type, int protocol, SOCKET descriptor, bool connected); // copy socket
			Sock(Sock &&other) noexcept;
			void operator=(Sock &&other) noexcept;
			~Sock();

			Sock(const Sock &) = delete;
			void operator=(const Sock &other) = delete;

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

			bool operator<(const Sock &other) const // fast comparison
			{
				return descriptor < other.descriptor;
			}

			SOCKET getSocket() const { return descriptor; }
			int getFamily() const { return family; };
			int getType() const { return type; };
			int getProtocol() const { return protocol; };
			bool getConnected() const { return connected; }

		private:
			SOCKET descriptor = INVALID_SOCKET;
			int family = -1, type = -1, protocol = -1;
			bool connected = false;
		};
	}
}

#endif // guard_net_h_7e42e43f_d30e_43ab_a4e4_a908d0a57f7a_
