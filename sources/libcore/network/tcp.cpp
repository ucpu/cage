#include "net.h"
#include <cage-core/lineReader.h>
#include <cage-core/math.h> // min

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

			TcpConnectionImpl(Sock &&s) : s(std::move(s))
			{}

			void waitForBytes(uintPtr size)
			{
				while (true)
				{
					uintPtr a = available();
					if (a >= size)
						break;
					// an ugly hack here, using the reserved capacity of a vector as a temporary storage
					if (buffer.capacity() < size)
						buffer.reserve(size);
					s.recv(buffer.data() + buffer.size(), size - a, MSG_WAITALL | MSG_PEEK); // blocking wait
				}
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

	string TcpConnection::address() const
	{
		const TcpConnectionImpl *impl = (const TcpConnectionImpl *)this;
		string a;
		uint16 p;
		impl->s.getRemoteAddress().translate(a, p);
		return a;
	}

	uint16 TcpConnection::port() const
	{
		const TcpConnectionImpl *impl = (const TcpConnectionImpl *)this;
		string a;
		uint16 p;
		impl->s.getRemoteAddress().translate(a, p);
		return p;
	}

	uintPtr TcpConnection::available()
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl *)this;
		uintPtr a = impl->s.available();
		if (a > 0)
		{
			uintPtr c = impl->buffer.size();
			impl->buffer.resize(c + a);
			impl->s.recv(&impl->buffer[c], a);
		}
		return impl->buffer.size();
	}

	void TcpConnection::readWait(PointerRange<char> buffer)
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl *)this;
		impl->waitForBytes(buffer.size());
		detail::memcpy(buffer.data(), impl->buffer.data(), buffer.size());
		detail::memmove(impl->buffer.data(), impl->buffer.data() + (buffer.size() + 1), impl->buffer.size() - buffer.size());
		impl->buffer.resize(impl->buffer.size() - buffer.size());
	}

	void TcpConnection::read(PointerRange<char> &buffer)
	{
		uintPtr s = min(buffer.size(), available());
		buffer = { buffer.data(), buffer.data() + s };
		readWait(buffer);
	}

	Holder<PointerRange<char>> TcpConnection::readWait(uintPtr size)
	{
		MemoryBuffer b(size);
		readWait(b);
		return std::move(b);
	}

	Holder<PointerRange<char>> TcpConnection::read()
	{
		MemoryBuffer b(available());
		readWait(b);
		return std::move(b);
	}

	string TcpConnection::readLineWait()
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl *)this;
		string line;
		while (!readLine(line))
			impl->waitForBytes(impl->buffer.size() + 1);
		return line;
	}

	bool TcpConnection::readLine(string &line)
	{
		available();
		TcpConnectionImpl *impl = (TcpConnectionImpl *)this;
		if (impl->buffer.empty())
			return false;

		PointerRange<const char> pr = impl->buffer;
		if (!detail::readLine(line, pr, true))
			return false;
		detail::memmove(impl->buffer.data(), pr.data(), pr.size());
		impl->buffer.resize(pr.size());
		return true;
	}

	void TcpConnection::write(PointerRange<const char> buffer)
	{
		TcpConnectionImpl *impl = (TcpConnectionImpl *)this;
		impl->s.send(buffer.data(), buffer.size());
	}

	void TcpConnection::writeLine(const string &str)
	{
		string tmp = str + "\n";
		write({ tmp.c_str(), tmp.c_str() + tmp.length() });
	}

	uint16 TcpServer::port() const
	{
		const TcpServerImpl *impl = (const TcpServerImpl *)this;
		string a;
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
					return systemArena().createImpl<TcpConnection, TcpConnectionImpl>(std::move(s));
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
		return systemArena().createImpl<TcpConnection, TcpConnectionImpl>(address, port);
	}

	Holder<TcpServer> newTcpServer(uint16 port)
	{
		return systemArena().createImpl<TcpServer, TcpServerImpl>(port);
	}
}
