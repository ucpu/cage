#include "net.h"
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
#endif
		}
	}

	namespace privat
	{
		Addr::Addr() : addrlen(0)
		{
			networkInitialize();
			detail::memset(&storage, 0, sizeof(storage));
		}

		void Addr::translate(string &address, uint16 &port, bool domain) const
		{
			char nameBuf[string::MaxLength], portBuf[7];
			if (getnameinfo((sockaddr*)&storage, addrlen, nameBuf, string::MaxLength - 1, portBuf, 6, NI_NUMERICSERV | (domain ? 0 : NI_NUMERICHOST)) != 0)
				CAGE_THROW_ERROR(SystemError, "translate address to human readable format failed (getnameinfo)", WSAGetLastError());
			address = string(nameBuf);
			port = numeric_cast<uint16>(toUint32(string(portBuf)));
		}

		Sock::Sock() : descriptor(INVALID_SOCKET), family(-1), type(-1), protocol(-1), connected(false)
		{}

		Sock::Sock(int family, int type, int protocol) : descriptor(INVALID_SOCKET), family(family), type(type), protocol(protocol), connected(false)
		{
			networkInitialize();
			descriptor = socket(family, type, protocol);
			if (descriptor == INVALID_SOCKET)
				CAGE_THROW_ERROR(SystemError, "socket creation failed (socket)", WSAGetLastError());
		}

		Sock::Sock(int family, int type, int protocol, SOCKET desc, bool connected) : descriptor(desc), family(family), type(type), protocol(protocol), connected(connected)
		{}

		Sock::Sock(Sock &&other) noexcept : descriptor(other.descriptor), family(other.family), type(other.type), protocol(other.protocol), connected(other.connected)
		{
			other.descriptor = INVALID_SOCKET;
		}

		void Sock::operator = (Sock &&other) noexcept
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
			if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, (raw_type*)&b, sizeof(b)) != 0)
				CAGE_THROW_ERROR(SystemError, "setting reuseaddr mode (setsockopt)", WSAGetLastError());
#ifndef CAGE_SYSTEM_WINDOWS
			if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEPORT, (raw_type*)&b, sizeof(b)) != 0)
				CAGE_THROW_ERROR(SystemError, "setting reuseport mode (setsockopt)", WSAGetLastError());
#endif
		}

		void Sock::setBroadcast(bool broadcast)
		{
			int broadcastPermission = broadcast ? 1 : 0;
			if (setsockopt(descriptor, SOL_SOCKET, SO_BROADCAST, (raw_type*)&broadcastPermission, sizeof(broadcastPermission)) != 0)
				CAGE_THROW_ERROR(SystemError, "setting broadcast mode (setsockopt)", WSAGetLastError());
		}

		void Sock::setBufferSize(uint32 sending, uint32 receiving)
		{
			const auto &fnc = [&](int optname, sint32 request)
			{
				socklen_t len = sizeof(sint32);
				sint32 cur = 0;
				if (getsockopt(descriptor, SOL_SOCKET, optname, (raw_type*)&cur, &len) != 0)
					CAGE_THROW_ERROR(SystemError, "retrieving buffer size (getsockopt)", WSAGetLastError());
				CAGE_ASSERT(len == sizeof(sint32));
				sint32 last = request;
				sint32 r = request;
				while (cur != last && cur < r)
				{
					CAGE_ASSERT(r > 0);
					if (setsockopt(descriptor, SOL_SOCKET, optname, (raw_type*)&r, len) != 0)
						CAGE_THROW_ERROR(SystemError, "setting buffer size (setsockopt)", WSAGetLastError());
					last = cur;
					if (getsockopt(descriptor, SOL_SOCKET, optname, (raw_type*)&cur, &len) != 0)
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
			if (::bind(descriptor, (sockaddr*)&localAddress, localAddress.addrlen) < 0)
				CAGE_THROW_ERROR(SystemError, "setting local address and port failed (bind)", WSAGetLastError());
		}

		void Sock::connect(const Addr &remoteAddress)
		{
			if (::connect(descriptor, (sockaddr*)&remoteAddress.storage, remoteAddress.addrlen) < 0)
				CAGE_THROW_SILENT(SystemError, "connect failed (connect)", WSAGetLastError());
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
			if (getsockname(descriptor, (sockaddr*)&a.storage, (socklen_t*)&a.addrlen) < 0)
				CAGE_THROW_ERROR(SystemError, "fetch of local address failed (getsockname)", WSAGetLastError());
			return a;
		}

		Addr Sock::getRemoteAddress() const
		{
			Addr a;
			a.addrlen = sizeof(a.storage);
			if (getpeername(descriptor, (sockaddr*)&a.storage, (socklen_t*)&a.addrlen) < 0)
				CAGE_THROW_ERROR(SystemError, "fetch of foreign address failed (getpeername)", WSAGetLastError());
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

		void Sock::send(const void *buffer, uintPtr bufferSize)
		{
			CAGE_ASSERT(connected);
			if (::send(descriptor, (raw_type*)buffer, numeric_cast<int>(bufferSize), 0) != bufferSize)
				CAGE_THROW_ERROR(SystemError, "send failed (send)", WSAGetLastError());
		}

		void Sock::sendTo(const void *buffer, uintPtr bufferSize, const Addr &remoteAddress)
		{
			CAGE_ASSERT(!connected);
			if (::sendto(descriptor, (raw_type*)buffer, numeric_cast<int>(bufferSize), 0, (sockaddr*)&remoteAddress.storage, remoteAddress.addrlen) != bufferSize)
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

		AddrList::AddrList(const string &address, uint16 port, int family, int type, int protocol, int flags) : AddrList(address.c_str(), port, family, type, protocol, flags)
		{}

		AddrList::AddrList(const char *address, uint16 port, int family, int type, int protocol, int flags) : start(nullptr), current(nullptr)
		{
			networkInitialize();
			addrinfo hints;
			detail::memset(&hints, 0, sizeof(hints));
			hints.ai_family = family;
			hints.ai_socktype = type;
			hints.ai_protocol = protocol;
			hints.ai_flags = flags;
			if (getaddrinfo(address, string(stringizer() + port).c_str(), &hints, &start) != 0)
				CAGE_THROW_ERROR(SystemError, "list available interfaces failed (getaddrinfo)", WSAGetLastError());
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

		void AddrList::getAll(Addr &address, int &family, int &type, int &protocol) const
		{
			CAGE_ASSERT(valid());
			detail::memcpy(&address.storage, current->ai_addr, current->ai_addrlen);
			address.addrlen = numeric_cast<socklen_t>(current->ai_addrlen);
			family = current->ai_family;
			type = current->ai_socktype;
			protocol = current->ai_protocol;
		}

		void AddrList::next()
		{
			CAGE_ASSERT(valid());
			current = current->ai_next;
		}
	}
}
