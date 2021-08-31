#include <cage-core/networkTcp.h>
#include <cage-core/lineReader.h>
#include <cage-core/math.h> // min

#include "net.h"

#include <vector>

namespace cage
{
	using namespace privat;

	namespace
	{
		class TcpConnectionImpl : public TcpConnection
		{
		public:
			Sock sock;
			std::vector<char> staging;

			TcpConnectionImpl(const String &address, uint16 port)
			{
				AddrList l(address, port, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0);
				while (l.valid())
				{
					detail::OverrideBreakpoint OverrideBreakpoint;
					int family = -1, type = -1, protocol = -1;
					Addr address;
					l.getAll(address, family, type, protocol);
					try
					{
						sock = Sock(family, type, protocol);
						sock.connect(address);
						break;
					}
					catch (const Exception &)
					{
						sock = Sock();
					}
					l.next();
				}

				if (!sock.isValid())
					CAGE_THROW_ERROR(Exception, "no connection");
			}

			TcpConnectionImpl(Sock &&sock) : sock(std::move(sock))
			{
				this->sock.setBlocking(true);
			}

			void update()
			{
				const uintPtr a = sock.available();
				if (a > 0)
				{
					const uintPtr c = staging.size();
					staging.resize(c + a);
					const auto r = sock.recv(&staging[c], a);
					if (r == 0)
						CAGE_THROW_ERROR(Disconnected, "disconnected");
					if (r != a)
						CAGE_THROW_ERROR(Exception, "incomplete read");
				}
			}

			void updateCheck()
			{
				if (staging.empty() && (sock.events() & (POLLIN | POLLERR | POLLHUP)) != 0 && sock.available() == 0)
					CAGE_THROW_ERROR(Disconnected, "disconnected");
				update();
			}

			void waitForBytes(uintPtr sz)
			{
				CAGE_ASSERT(sz > 0);
				while (true)
				{
					update();
					const uintPtr a = staging.size();
					if (a >= sz)
						break;
					// an ugly hack here, using the reserved capacity of a vector as a temporary storage
					if (staging.capacity() < sz)
						staging.reserve(sz);
					if (!sock.recv(staging.data() + staging.size(), sz - a, MSG_PEEK)) // blocking
						CAGE_THROW_ERROR(Disconnected, "disconnected");
				}
			}

			void discardBytes(uintPtr sz)
			{
				CAGE_ASSERT(staging.size() >= sz);
				const uintPtr ts = staging.size() - sz;
				detail::memmove(staging.data(), staging.data() + sz, ts);
				staging.resize(ts);
			}

			void read(PointerRange<char> data) override
			{
				detail::OverrideBreakpoint brk;
				waitForBytes(data.size());
				CAGE_ASSERT(staging.size() >= data.size());
				detail::memcpy(data.data(), staging.data(), data.size());
				discardBytes(data.size());
			}

			void write(PointerRange<const char> data) override
			{
				detail::OverrideBreakpoint brk;
				sock.send(data.data(), data.size());
			}

			uintPtr size() override
			{
				detail::OverrideBreakpoint brk;
				updateCheck();
				return staging.size();
			}

			String readLine() override
			{
				detail::OverrideBreakpoint brk;
				updateCheck();
				const uintPtr sz = staging.size();
				String line;
				const uintPtr rd = detail::readLine(line, staging, true);
				if (rd)
				{
					discardBytes(rd);
					return line;
				}
				waitForBytes(sz + 1); // blocking call
				return readLine(); // try again
			}

			bool readLine(String &line) override
			{
				detail::OverrideBreakpoint brk;
				updateCheck();
				const uintPtr rd = detail::readLine(line, staging, true);
				if (rd)
					discardBytes(rd);
				return rd;
			}
		};

		class TcpServerImpl : public TcpServer
		{
		public:
			std::vector<Sock> socks;

			TcpServerImpl(uint16 port)
			{
				AddrList l(nullptr, port, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, AI_PASSIVE);
				while (l.valid())
				{
					int family = -1, type = -1, protocol = -1;
					Addr address;
					l.getAll(address, family, type, protocol);
					try
					{
						Sock s(family, type, protocol);
						s.setBlocking(false);
						s.setReuseaddr(true);
						s.bind(address);
						s.listen();
						socks.push_back(std::move(s));
					}
					catch (const Exception &)
					{
						// do nothing
					}
					l.next();
				}

				if (socks.empty())
					CAGE_THROW_ERROR(Exception, "no interface");
			}
		};
	}

	String TcpConnection::address() const
	{
		const TcpConnectionImpl *impl = (const TcpConnectionImpl *)this;
		String a;
		uint16 p;
		impl->sock.getRemoteAddress().translate(a, p);
		return a;
	}

	uint16 TcpConnection::port() const
	{
		const TcpConnectionImpl *impl = (const TcpConnectionImpl *)this;
		String a;
		uint16 p;
		impl->sock.getRemoteAddress().translate(a, p);
		return p;
	}

	uint16 TcpServer::port() const
	{
		const TcpServerImpl *impl = (const TcpServerImpl *)this;
		String a;
		uint16 p;
		impl->socks[0].getLocalAddress().translate(a, p);
		return p;
	}

	Holder<TcpConnection> TcpServer::accept()
	{
		TcpServerImpl *impl = (TcpServerImpl *)this;

		for (Sock &ss : impl->socks)
		{
			try
			{
				Sock s = ss.accept();
				if (s.isValid())
					return systemMemory().createImpl<TcpConnection, TcpConnectionImpl>(std::move(s));
			}
			catch (const cage::Exception &)
			{
				// do nothing
			}
		}

		return Holder<TcpConnection>();
	}

	Holder<TcpConnection> newTcpConnection(const String &address, uint16 port)
	{
		return systemMemory().createImpl<TcpConnection, TcpConnectionImpl>(address, port);
	}

	Holder<TcpServer> newTcpServer(uint16 port)
	{
		return systemMemory().createImpl<TcpServer, TcpServerImpl>(port);
	}
}
