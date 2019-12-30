#include "net.h"
#include <cage-core/lineReader.h>

#include <vector>

namespace cage
{
	using namespace privat;

	namespace
	{
		class TcpConnectionImpl : public TcpConnection
		{
		public:
			Sock s;
			std::vector<char> buffer;

			TcpConnectionImpl(const string &address, uint16 port)
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
						s = Sock(family, type, protocol);
						s.connect(address);
						break;
					}
					catch (const Exception &)
					{
						s = Sock();
					}
					l.next();
				}

				if (!s.isValid())
					CAGE_THROW_ERROR(Exception, "no connection");

				s.setBlocking(false);
			}

			TcpConnectionImpl(Sock &&s) : s(templates::move(s))
			{}
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
						socks.push_back(templates::move(s));
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

	string TcpConnection::address() const
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl*)this;
		string a;
		uint16 p;
		impl->s.getRemoteAddress().translate(a, p);
		return a;
	}

	uint16 TcpConnection::port() const
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl*)this;
		string a;
		uint16 p;
		impl->s.getRemoteAddress().translate(a, p);
		return p;
	}

	uintPtr TcpConnection::available() const
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl*)this;
		uintPtr a = impl->s.available();
		if (a > 0)
		{
			uintPtr c = impl->buffer.size();
			impl->buffer.resize(c + a);
			impl->s.recv(&impl->buffer[c], a);
		}
		return impl->buffer.size();
	}

	void TcpConnection::read(void *buffer, uintPtr size)
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl*)this;
		uintPtr a = available();
		if (size > a)
		{
			impl->s.recv(buffer, size - a, MSG_WAITALL | MSG_PEEK); // blocking wait
			return read(buffer, size);
		}
		detail::memcpy(buffer, &impl->buffer[0], size);
		detail::memmove(&impl->buffer[0], &impl->buffer[size + 1], impl->buffer.size() - size);
		impl->buffer.resize(impl->buffer.size() - size);
	}

	MemoryBuffer TcpConnection::read(uintPtr size)
	{
		MemoryBuffer b(size);
		read(b.data(), b.size());
		return b;
	}

	MemoryBuffer TcpConnection::read()
	{
		MemoryBuffer b(available());
		read(b.data(), b.size());
		return b;
	}

	void TcpConnection::write(const void *buffer, uintPtr size)
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl*)this;
		impl->s.send(buffer, size);
	}

	void TcpConnection::write(const MemoryBuffer &buffer)
	{
		write(buffer.data(), buffer.size());
	}

	bool TcpConnection::readLine(string &line)
	{
		available();
		TcpConnectionImpl *impl = (TcpConnectionImpl*)this;
		if (impl->buffer.empty())
			return false;

		const char *b = impl->buffer.data();
		uintPtr s = impl->buffer.size();
		if (!detail::readLine(line, b, s, true))
			return false;
		detail::memmove(impl->buffer.data(), b, s);
		impl->buffer.resize(s);
		return true;
	}

	void TcpConnection::writeLine(const string &str)
	{
		string tmp = str + "\n";
		write((void*)tmp.c_str(), tmp.length());
	}

	uint16 TcpServer::port() const
	{
		TcpServerImpl *impl = (TcpServerImpl*)this;
		string a;
		uint16 p;
		impl->socks[0].getLocalAddress().translate(a, p);
		return p;
	}

	Holder<TcpConnection> TcpServer::accept()
	{
		TcpServerImpl *impl = (TcpServerImpl*)this;

		for (Sock &ss : impl->socks)
		{
			try
			{
				Sock s = ss.accept();
				if (s.isValid())
					return detail::systemArena().createImpl<TcpConnection, TcpConnectionImpl>(templates::move(s));
			}
			catch (const cage::Exception &)
			{
				// do nothing
			}
		}

		return Holder<TcpConnection>();
	}

	Holder<TcpConnection> newTcpConnection(const string &address, uint16 port)
	{
		return detail::systemArena().createImpl<TcpConnection, TcpConnectionImpl>(address, port);
	}

	Holder<TcpServer> newTcpServer(uint16 port)
	{
		return detail::systemArena().createImpl<TcpServer, TcpServerImpl>(port);
	}
}
