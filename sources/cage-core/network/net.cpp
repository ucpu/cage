#include "net.h"

namespace cage
{
	namespace privat
	{
		addr::addr() : addrlen(0)
		{
			detail::memset(&storage, 0, sizeof(storage));
		}

		void addr::translate(string &address, uint16 &port, bool domain) const
		{
			char nameBuf[string::MaxLength], portBuf[7];
			if (getnameinfo((sockaddr*)&storage, addrlen, nameBuf, string::MaxLength - 1, portBuf, 6, NI_NUMERICSERV | (domain ? 0 : NI_NUMERICHOST)) != 0)
				CAGE_THROW_ERROR(codeException, "translate address to human readable format failed (getnameinfo)", WSAGetLastError());
			address = string(nameBuf);
			port = numeric_cast<uint16>(string(portBuf).toUint32());
		}

		sock::sock(SOCKET descriptor) : descriptor(descriptor)
		{}

		sock::sock(int family, int type, int protocol)
		{
			descriptor = socket(family, type, protocol);
			if (descriptor == INVALID_SOCKET)
				CAGE_THROW_ERROR(codeException, "socket creation failed (socket)", WSAGetLastError());
		}

		sock::sock(sock &&other) noexcept : descriptor(other.descriptor)
		{
			other.descriptor = INVALID_SOCKET;
		}

		void sock::operator = (sock &&other) noexcept
		{
			if (this == &other)
				return;
			if (isValid())
				::closesocket(descriptor);
			descriptor = other.descriptor;
			other.descriptor = INVALID_SOCKET;
		}

		sock::~sock()
		{
			if (isValid())
				::closesocket(descriptor);
			descriptor = -1;
		}

		void sock::setBlocking(bool blocking)
		{
			u_long b = blocking ? 0 : 1;
			if (ioctlsocket(descriptor, FIONBIO, &b) != 0)
				CAGE_THROW_ERROR(codeException, "setting (non)blocking mode (ioctlsocket)", WSAGetLastError());
		}

		void sock::setReuseaddr(bool reuse)
		{
			int b = reuse ? 1 : 0;
			if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, (raw_type*)&b, sizeof(b)) != 0)
				CAGE_THROW_ERROR(codeException, "setting reuseaddr mode (setsockopt)", WSAGetLastError());
#ifndef CAGE_SYSTEM_WINDOWS
			if (setsockopt(descriptor, SOL_SOCKET, SO_REUSEPORT, (raw_type*)&b, sizeof(b)) != 0)
				CAGE_THROW_ERROR(codeException, "setting reuseport mode (setsockopt)", WSAGetLastError());
#endif
		}

		void sock::setBroadcast(bool broadcast)
		{
			int broadcastPermission = broadcast ? 1 : 0;
			if (setsockopt(descriptor, SOL_SOCKET, SO_BROADCAST, (raw_type*)&broadcastPermission, sizeof(broadcastPermission)) != 0)
				CAGE_THROW_ERROR(codeException, "setting broadcast mode (setsockopt)", WSAGetLastError());
		}

		void sock::bind(const addr &localAddress)
		{
			if (::bind(descriptor, (sockaddr*)&localAddress, localAddress.addrlen) < 0)
				CAGE_THROW_ERROR(codeException, "setting local address and port failed (bind)", WSAGetLastError());
		}

		void sock::connect(const addr &remoteAddress)
		{
			if (::connect(descriptor, (sockaddr*)&remoteAddress.storage, remoteAddress.addrlen) < 0)
				CAGE_THROW_SILENT(codeException, "connect failed (connect)", WSAGetLastError());
		}

		void sock::listen(int backlog)
		{
			if (::listen(descriptor, backlog) < 0)
				CAGE_THROW_ERROR(codeException, "setting listening socket failed (listen)", WSAGetLastError());
		}

		sock sock::accept()
		{
			SOCKET rtn;
			if ((rtn = ::accept(descriptor, nullptr, 0)) == INVALID_SOCKET)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					CAGE_THROW_ERROR(codeException, "accept failed (accept)", err);
				rtn = -1;
			}
			return sock(rtn);
		}

		bool sock::isValid() const
		{
			return descriptor != (SOCKET)-1;
		}

		addr sock::getLocalAddress() const
		{
			addr a;
			a.addrlen = sizeof(a.storage);
			if (getsockname(descriptor, (sockaddr*)&a.storage, (socklen_t*)&a.addrlen) < 0)
				CAGE_THROW_ERROR(codeException, "fetch of local address failed (getsockname)", WSAGetLastError());
			return a;
		}

		addr sock::getRemoteAddress() const
		{
			addr a;
			a.addrlen = sizeof(a.storage);
			if (getpeername(descriptor, (sockaddr*)&a.storage, (socklen_t*)&a.addrlen) < 0)
				CAGE_THROW_ERROR(codeException, "fetch of foreign address failed (getpeername)", WSAGetLastError());
			return a;
		}

		uintPtr sock::available() const
		{
			u_long res = 0;
			if (ioctlsocket(descriptor, FIONREAD, &res) != 0)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					CAGE_THROW_ERROR(codeException, "determine available bytes (ioctlsocket)", err);
			}
			return res;
		}

		void sock::send(void *buffer, uintPtr bufferSize)
		{
			if (::send(descriptor, (raw_type*)buffer, numeric_cast<int>(bufferSize), 0) != bufferSize)
				CAGE_THROW_ERROR(codeException, "send failed (send)", WSAGetLastError());
		}

		void sock::sendTo(void *buffer, uintPtr bufferSize, const addr &remoteAddress)
		{
			if (::sendto(descriptor, (raw_type*)buffer, numeric_cast<int>(bufferSize), 0, (sockaddr*)&remoteAddress.storage, remoteAddress.addrlen) != bufferSize)
				CAGE_THROW_ERROR(codeException, "send failed (sendto)", WSAGetLastError());
		}

		uintPtr sock::recv(void *buffer, uintPtr bufferSize, int flags)
		{
			int rtn;
			if ((rtn = ::recv(descriptor, (raw_type*)buffer, numeric_cast<int>(bufferSize), flags)) < 0)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					CAGE_THROW_ERROR(codeException, "received failed (recv)", err);
				rtn = 0;
			}
			return rtn;
		}

		uintPtr sock::recvFrom(void *buffer, uintPtr bufferSize, addr &remoteAddress, int flags)
		{
			remoteAddress.addrlen = sizeof(remoteAddress.storage);
			int rtn;
			if ((rtn = ::recvfrom(descriptor, (raw_type*)buffer, numeric_cast<int>(bufferSize), flags, (sockaddr*)&remoteAddress.storage, &remoteAddress.addrlen)) < 0)
			{
				int err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK)
					CAGE_THROW_ERROR(codeException, "received failed (recvfrom)", err);
				rtn = 0;
			}
			return rtn;
		}

		addrList::addrList(const char *address, uint16 port, int family, int type, int protocol, int flags) : start(nullptr), current(nullptr)
		{
			addrinfo hints;
			detail::memset(&hints, 0, sizeof(hints));
			hints.ai_family = family;
			hints.ai_socktype = type;
			hints.ai_protocol = protocol;
			hints.ai_flags = flags;
			if (getaddrinfo(address, string(port).c_str(), &hints, &start) != 0)
				CAGE_THROW_ERROR(codeException, "list available interfaces failed (getaddrinfo)", WSAGetLastError());
			current = start;
		}

		addrList::~addrList()
		{
			if (start)
				freeaddrinfo(start);
			start = current = nullptr;
		}

		bool addrList::valid() const
		{
			return !!current;
		}

		addr addrList::address() const
		{
			CAGE_ASSERT_RUNTIME(valid());
			addr address;
			detail::memcpy(&address.storage, current->ai_addr, current->ai_addrlen);
			address.addrlen = numeric_cast<socklen_t>(current->ai_addrlen);
			return address;
		}

		int addrList::family() const
		{
			CAGE_ASSERT_RUNTIME(valid());
			return current->ai_family;
		}

		int addrList::tyoe() const
		{
			CAGE_ASSERT_RUNTIME(valid());
			return current->ai_socktype;
		}

		int addrList::protocol() const
		{
			CAGE_ASSERT_RUNTIME(valid());
			return current->ai_protocol;
		}

		void addrList::getAll(addr &address, int &family, int &type, int &protocol) const
		{
			CAGE_ASSERT_RUNTIME(valid());
			detail::memcpy(&address.storage, current->ai_addr, current->ai_addrlen);
			address.addrlen = numeric_cast<socklen_t>(current->ai_addrlen);
			family = current->ai_family;
			type = current->ai_socktype;
			protocol = current->ai_protocol;
		}

		void addrList::next()
		{
			CAGE_ASSERT_RUNTIME(valid());
			current = current->ai_next;
		}
	}

#ifdef CAGE_SYSTEM_WINDOWS
	namespace
	{
		class networkInitializerClass
		{
		public:
			networkInitializerClass()
			{
				WSADATA wsaData;
				int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (err != 0)
					CAGE_THROW_CRITICAL(codeException, "WSAStartup", err);
			}

			~networkInitializerClass()
			{
				WSACleanup();
			}
		} networkInitializerInstance;
	}
#endif
}
