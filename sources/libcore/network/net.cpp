#include "net.h"

#include <cage-core/endianness.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>

namespace cage
{
	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		bool networkInitializeImpl()
		{
			WSADATA wsaData;
			int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (err != 0)
				CAGE_THROW_ERROR(SystemError, "WSAStartup", err);
			return true;
		}
#endif

		void networkInitialize()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			static const bool init = networkInitializeImpl();
			(void)init;
#endif
		}
	}

	namespace privat
	{
		Addr::Addr(std::array<uint8, 4> ipv4, uint16 port)
		{
			sockaddr_in &in = (sockaddr_in &)storage;
			in.sin_family = AF_INET;
			in.sin_port = port;
			static_assert(sizeof(in.sin_addr) == sizeof(ipv4));
			detail::memcpy(&in.sin_addr, &ipv4, sizeof(ipv4));
			in.sin_port = endianness::change(in.sin_port);
			in.sin_addr = endianness::change(in.sin_addr);
			addrlen = sizeof(sockaddr_in);
		}

		Addr::Addr(std::array<uint8, 16> ipv6, uint16 port)
		{
			sockaddr_in6 &in = (sockaddr_in6 &)storage;
			in.sin6_family = AF_INET6;
			in.sin6_port = port;
			static_assert(sizeof(in.sin6_addr) == sizeof(ipv6));
			detail::memcpy(&in.sin6_addr, &ipv6, sizeof(ipv6));
			in.sin6_port = endianness::change(in.sin6_port);
			in.sin6_addr = endianness::change(in.sin6_addr);
			addrlen = sizeof(sockaddr_in6);
		}

		std::pair<String, uint16> Addr::translate(bool domain) const
		{
			networkInitialize();
			char nameBuf[String::MaxLength], portBuf[7];
			if (getnameinfo((sockaddr *)&storage, addrlen, nameBuf, String::MaxLength - 1, portBuf, 6, NI_NUMERICSERV | (domain ? 0 : NI_NUMERICHOST)) != 0)
				CAGE_THROW_ERROR(SystemError, "translate address to human readable format failed (getnameinfo)", WSAGetLastError());
			std::pair<String, uint16> res;
			res.first = String(nameBuf);
			res.second = numeric_cast<uint16>(toUint32(String(portBuf)));
			return res;
		}

		std::array<uint8, 4> Addr::getIp4() const
		{
			CAGE_ASSERT(storage.ss_family == AF_INET);
			std::array<uint8, 4> a;
			const sockaddr_in &in = (const sockaddr_in &)storage;
			detail::memcpy(a.data(), &in.sin_addr, 4);
			return endianness::change(a);
		}

		std::array<uint8, 16> Addr::getIp6() const
		{
			CAGE_ASSERT(storage.ss_family == AF_INET6);
			std::array<uint8, 16> a;
			const sockaddr_in6 &in = (const sockaddr_in6 &)storage;
			detail::memcpy(a.data(), &in.sin6_addr, 16);
			return endianness::change(a);
		}

		uint16 Addr::getPort() const
		{
			CAGE_ASSERT(storage.ss_family == AF_INET || storage.ss_family == AF_INET6);
			if (storage.ss_family == AF_INET6)
			{
				const sockaddr_in6 &in = (const sockaddr_in6 &)storage;
				return endianness::change(in.sin6_port);
			}
			else
			{
				const sockaddr_in &in = (const sockaddr_in &)storage;
				return endianness::change(in.sin_port);
			}
		}

		struct Test
		{
			bool testing()
			{
				constexpr auto ip4 = std::array<uint8, 4>{ 10, 20, 30, 40 };
				constexpr auto ip6 = std::array<uint8, 16>{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
				if (Addr(ip4, 1234).getIp4() != ip4)
					return false;
				if (Addr(ip4, 1234).getPort() != 1234)
					return false;
				if (Addr(ip6, 1234).getIp6() != ip6)
					return false;
				return true;
			}

			Test() { CAGE_ASSERT(testing()); }
		} testInstance;

		Serializer &operator<<(Serializer &s, const Addr &v)
		{
			switch (v.storage.ss_family)
			{
				case AF_INET:
				{
					CAGE_ASSERT(v.addrlen == sizeof(sockaddr_in));
					const sockaddr_in &in = (const sockaddr_in &)v.storage;
					s << (uint16)1;
					s << (uint16)in.sin_port;
					static_assert(sizeof(in.sin_addr) == 4);
					s << in.sin_addr;
					return s;
				}
				case AF_INET6:
				{
					CAGE_ASSERT(v.addrlen == sizeof(sockaddr_in6));
					const sockaddr_in6 &in = (const sockaddr_in6 &)v.storage;
					s << (uint16)2;
					s << (uint16)in.sin6_port;
					static_assert(sizeof(in.sin6_addr) == 16);
					s << in.sin6_addr;
					return s;
				}
				default:
					CAGE_THROW_ERROR(Exception, "cannot serialize Addr - unknown ip family");
			}
		}

		Deserializer &operator>>(Deserializer &s, Addr &v)
		{
			uint16 f, p;
			s >> f >> p;
			switch (f)
			{
				case 1:
				{
					std::array<uint8, 4> ip;
					s >> ip;
					v = Addr(endianness::change(ip), endianness::change(p));
					return s;
				}
				case 2:
				{
					std::array<uint8, 16> ip;
					s >> ip;
					v = Addr(endianness::change(ip), endianness::change(p));
					return s;
				}
				default:
					CAGE_THROW_ERROR(Exception, "cannot deserialize Addr - unknown ip family");
			}
		}

		AddrList::AddrList(const String &address, uint16 port, int family, int type, int protocol, int flags) : AddrList(address.c_str(), port, family, type, protocol, flags) {}

		AddrList::AddrList(const char *address, uint16 port, int family, int type, int protocol, int flags) : start(nullptr), current(nullptr)
		{
			networkInitialize();
			addrinfo hints;
			detail::memset(&hints, 0, sizeof(hints));
			hints.ai_family = family;
			hints.ai_socktype = type;
			hints.ai_protocol = protocol;
			hints.ai_flags = flags;
			if (auto err = getaddrinfo(address, String(Stringizer() + port).c_str(), &hints, &start) != 0)
				CAGE_THROW_ERROR(SystemError, "list available interfaces failed (getaddrinfo)", err);
			current = start;
		}

		AddrList::~AddrList()
		{
			if (start)
				freeaddrinfo(start);
			start = current = nullptr;
		}

		bool AddrList::valid() const
		{
			return !!current;
		}

		void AddrList::next()
		{
			CAGE_ASSERT(valid());
			current = current->ai_next;
		}

		Sock AddrList::sock() const
		{
			CAGE_ASSERT(valid());
			return Sock(family(), type(), protocol());
		}

		Addr AddrList::address() const
		{
			CAGE_ASSERT(valid());
			Addr address;
			detail::memcpy(&address.storage, current->ai_addr, current->ai_addrlen);
			address.addrlen = numeric_cast<socklen_t>(current->ai_addrlen);
			return address;
		}

		int AddrList::family() const
		{
			CAGE_ASSERT(valid());
			return current->ai_family;
		}

		int AddrList::type() const
		{
			CAGE_ASSERT(valid());
			return current->ai_socktype;
		}

		int AddrList::protocol() const
		{
			CAGE_ASSERT(valid());
			return current->ai_protocol;
		}

		Sock::Sock() {}

		Sock::Sock(int family, int type, int protocol) : family(family), type(type), protocol(protocol)
		{
			networkInitialize();
#ifdef CAGE_SYSTEM_WINDOWS
			descriptor = socket(family, type, protocol);
#elif defined(CAGE_SYSTEM_LINUX)
			// SOCK_CLOEXEC -> close the socket on exec
			descriptor = socket(family, type | SOCK_CLOEXEC, protocol);
#elif defined(CAGE_SYSTEM_MAC)
			descriptor = socket(family, type, protocol);
			// FD_CLOEXEC -> close the socket on exec
			fcntl(descriptor, F_SETFD, FD_CLOEXEC);
#else
	#error This operating system is not supported
#endif
			if (descriptor == INVALID_SOCKET)
				CAGE_THROW_ERROR(SystemError, "socket creation failed (socket)", WSAGetLastError());
		}

		Sock::Sock(int family, int type, int protocol, SOCKET desc, bool connected) : descriptor(desc), family(family), type(type), protocol(protocol), connected(connected) {}

		Sock::Sock(Sock &&other) : descriptor(other.descriptor), family(other.family), type(other.type), protocol(other.protocol), connected(other.connected)
		{
			other.descriptor = INVALID_SOCKET;
		}

		void Sock::operator=(Sock &&other)
		{
			if (this == &other)
				return;
			if (isValid())
				::closesocket(descriptor);
			descriptor = other.descriptor;
			family = other.family;
			type = other.type;
			protocol = other.protocol;
			connected = other.connected;
			other.descriptor = INVALID_SOCKET;
		}

		Sock::~Sock()
		{
			close();
		}

		void Sock::setBlocking(bool blocking)
		{
			u_long b = blocking ? 0 : 1;
			if (ioctlsocket(descriptor, FIONBIO, &b) != 0)
				CAGE_THROW_ERROR(SystemError, "setting (non)blocking mode (ioctlsocket)", WSAGetLastError());
		}

		void Sock::setReuseaddr(bool reuse)
		{
			int b = reuse ? 1 : 0;
			if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, (raw_type *)&b, sizeof(b)) != 0)
				CAGE_THROW_ERROR(SystemError, "setting reuseaddr mode (setsockopt)", WSAGetLastError());
#ifndef CAGE_SYSTEM_WINDOWS
			if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEPORT, (raw_type *)&b, sizeof(b)) != 0)
				CAGE_THROW_ERROR(SystemError, "setting reuseport mode (setsockopt)", WSAGetLastError());
#endif
		}

		void Sock::setBroadcast(bool broadcast)
		{
			int broadcastPermission = broadcast ? 1 : 0;
			if (setsockopt(descriptor, SOL_SOCKET, SO_BROADCAST, (raw_type *)&broadcastPermission, sizeof(broadcastPermission)) != 0)
				CAGE_THROW_ERROR(SystemError, "setting broadcast mode (setsockopt)", WSAGetLastError());
		}

		void Sock::setBufferSize(uint32 sending, uint32 receiving)
		{
			const auto &fnc = [&](int optname, sint32 request)
			{
				socklen_t len = sizeof(sint32);
				sint32 cur = 0;
				if (getsockopt(descriptor, SOL_SOCKET, optname, (raw_type *)&cur, &len) != 0)
					CAGE_THROW_ERROR(SystemError, "retrieving buffer size (getsockopt)", WSAGetLastError());
				CAGE_ASSERT(len == sizeof(sint32));
				sint32 last = request;
				sint32 r = request;
				while (cur != last && cur < r)
				{
					CAGE_ASSERT(r > 0);
					if (setsockopt(descriptor, SOL_SOCKET, optname, (raw_type *)&r, len) != 0)
						CAGE_THROW_ERROR(SystemError, "setting buffer size (setsockopt)", WSAGetLastError());
					last = cur;
					if (getsockopt(descriptor, SOL_SOCKET, optname, (raw_type *)&cur, &len) != 0)
						CAGE_THROW_ERROR(SystemError, "retrieving buffer size (getsockopt)", WSAGetLastError());
					r -= 32768;
				}
				//CAGE_LOG(SeverityEnum::Info, "socket", stringizer() + "buffer request: " + request + ", current: " + cur + ", last: " + last);
			};
			fnc(SO_SNDBUF, numeric_cast<sint32>(sending));
			fnc(SO_RCVBUF, numeric_cast<sint32>(receiving));
		}

		void Sock::setBufferSize(uint32 size)
		{
			setBufferSize(size, size);
		}

		void Sock::bind(const Addr &localAddress)
		{
			if (::bind(descriptor, (sockaddr *)&localAddress, localAddress.addrlen) < 0)
				CAGE_THROW_ERROR(SystemError, "setting local address and port failed (bind)", WSAGetLastError());
		}

		void Sock::connect(const Addr &remoteAddress)
		{
			if (::connect(descriptor, (sockaddr *)&remoteAddress.storage, remoteAddress.addrlen) < 0)
				CAGE_THROW_ERROR(SystemError, "connect failed (connect)", WSAGetLastError());
			connected = true;
		}

		void Sock::listen(int backlog)
		{
			if (::listen(descriptor, backlog) < 0)
				CAGE_THROW_ERROR(SystemError, "setting listening socket failed (listen)", WSAGetLastError());
		}

		Sock Sock::accept()
		{
			SOCKET rtn = ::accept(descriptor, nullptr, 0);
			if (rtn == INVALID_SOCKET)
			{
				const int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					CAGE_THROW_ERROR(SystemError, "accept failed (accept)", err);
				rtn = INVALID_SOCKET;
			}
			return Sock(family, type, protocol, rtn, true);
		}

		void Sock::close()
		{
			if (isValid())
				::closesocket(descriptor);
			descriptor = INVALID_SOCKET;
		}

		bool Sock::isValid() const
		{
			return descriptor != INVALID_SOCKET;
		}

		Addr Sock::getLocalAddress() const
		{
			Addr a;
			a.addrlen = sizeof(a.storage);
			if (getsockname(descriptor, (sockaddr *)&a.storage, (socklen_t *)&a.addrlen) < 0)
				CAGE_THROW_ERROR(SystemError, "fetch of local address failed (getsockname)", WSAGetLastError());
			return a;
		}

		Addr Sock::getRemoteAddress() const
		{
			Addr a;
			a.addrlen = sizeof(a.storage);
			if (getpeername(descriptor, (sockaddr *)&a.storage, (socklen_t *)&a.addrlen) < 0)
				CAGE_THROW_ERROR(SystemError, "fetch of remote address failed (getpeername)", WSAGetLastError());
			return a;
		}

		uintPtr Sock::available() const
		{
			u_long res = 0;
			if (ioctlsocket(descriptor, FIONREAD, &res) != 0)
			{
				const int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					CAGE_THROW_ERROR(SystemError, "determine available bytes (ioctlsocket)", err);
			}
			return res;
		}

		uint16 Sock::events() const
		{
			pollfd fds = {};
			fds.fd = descriptor;
			fds.events = POLLIN | POLLOUT | POLLPRI | POLLRDHUP;
			const auto res = WSAPoll(&fds, 1, 0);
			if (res < 0)
				CAGE_THROW_ERROR(SystemError, "check socket events (WSAPoll)", WSAGetLastError());
			return fds.revents;
		}

		void Sock::send(const void *buffer, uintPtr bufferSize)
		{
			CAGE_ASSERT(connected);
			if (::send(descriptor, (raw_type *)buffer, numeric_cast<int>(bufferSize), 0) != bufferSize)
				CAGE_THROW_ERROR(SystemError, "send failed (send)", WSAGetLastError());
		}

		void Sock::sendTo(const void *buffer, uintPtr bufferSize, const Addr &remoteAddress)
		{
			CAGE_ASSERT(!connected);
			CAGE_ASSERT(remoteAddress.getFamily() == family);
			if (::sendto(descriptor, (raw_type *)buffer, numeric_cast<int>(bufferSize), 0, (sockaddr *)&remoteAddress.storage, remoteAddress.addrlen) != bufferSize)
				CAGE_THROW_ERROR(SystemError, "send failed (sendto)", WSAGetLastError());
		}

		uintPtr Sock::recv(void *buffer, uintPtr bufferSize, int flags)
		{
			CAGE_ASSERT(connected);
			const int rtn = ::recv(descriptor, (raw_type *)buffer, numeric_cast<int>(bufferSize), flags);
			if (rtn < 0)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK && (err != WSAECONNRESET || protocol != IPPROTO_UDP))
					CAGE_THROW_ERROR(SystemError, "received failed (recv)", err);
				return 0;
			}
			return rtn;
		}

		uintPtr Sock::recvFrom(void *buffer, uintPtr bufferSize, Addr &remoteAddress, int flags)
		{
			remoteAddress.addrlen = sizeof(remoteAddress.storage);
			const int rtn = ::recvfrom(descriptor, (raw_type *)buffer, numeric_cast<int>(bufferSize), flags, (sockaddr *)&remoteAddress.storage, &remoteAddress.addrlen);
			if (rtn < 0)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK && (err != WSAECONNRESET || protocol != IPPROTO_UDP))
					CAGE_THROW_ERROR(SystemError, "received failed (recvfrom)", err);
				return 0;
			}
			return rtn;
		}
	}
}
