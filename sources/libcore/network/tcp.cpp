#include <vector>
#include "net.h"

namespace cage
{
	using namespace privat;

	namespace
	{
		class tcpConnectionImpl : public tcpConnectionClass
		{
		public:
			sock s;
			std::vector<char> buffer;

			tcpConnectionImpl(const string &address, uint16 port)
			{
				addrList l(address, port, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0);
				while (l.valid())
				{
					detail::overrideBreakpoint overrideBreakpoint;
					int family = -1, type = -1, protocol = -1;
					addr address;
					l.getAll(address, family, type, protocol);
					try
					{
						s = sock(family, type, protocol);
						s.connect(address);
						break;
					}
					catch (const exception &)
					{
						s = sock();
					}
					l.next();
				}

				if (!s.isValid())
					CAGE_THROW_ERROR(exception, "no connection");

				s.setBlocking(false);
			}

			tcpConnectionImpl(sock &&s) : s(templates::move(s))
			{}
		};

		class tcpServerImpl : public tcpServerClass
		{
		public:
			std::vector<sock> socks;

			tcpServerImpl(uint16 port)
			{
				addrList l(nullptr, port, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, AI_PASSIVE);
				while (l.valid())
				{
					int family = -1, type = -1, protocol = -1;
					addr address;
					l.getAll(address, family, type, protocol);
					try
					{
						sock s(family, type, protocol);
						s.setBlocking(false);
						s.setReuseaddr(true);
						s.bind(address);
						s.listen();
						socks.push_back(templates::move(s));
					}
					catch (const exception &)
					{
						// do nothing
					}
					l.next();
				}

				if (socks.empty())
					CAGE_THROW_ERROR(exception, "no interface");
			}
		};
	}

	string tcpConnectionClass::address() const
	{
		tcpConnectionImpl *impl = (tcpConnectionImpl*)this;
		string a;
		uint16 p;
		impl->s.getRemoteAddress().translate(a, p);
		return a;
	}

	uint16 tcpConnectionClass::port() const
	{
		tcpConnectionImpl *impl = (tcpConnectionImpl*)this;
		string a;
		uint16 p;
		impl->s.getRemoteAddress().translate(a, p);
		return p;
	}

	uintPtr tcpConnectionClass::available() const
	{
		tcpConnectionImpl *impl = (tcpConnectionImpl*)this;
		uintPtr a = impl->s.available();
		if (a > 0)
		{
			uintPtr c = impl->buffer.size();
			impl->buffer.resize(c + a);
			impl->s.recv(&impl->buffer[c], a);
		}
		return impl->buffer.size();
	}

	void tcpConnectionClass::read(void *buffer, uintPtr size)
	{
		tcpConnectionImpl *impl = (tcpConnectionImpl*)this;
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

	memoryBuffer tcpConnectionClass::read(uintPtr size)
	{
		memoryBuffer b(size);
		read(b.data(), b.size());
		return b;
	}

	memoryBuffer tcpConnectionClass::read()
	{
		memoryBuffer b(available());
		read(b.data(), b.size());
		return b;
	}

	void tcpConnectionClass::write(const void *buffer, uintPtr size)
	{
		tcpConnectionImpl *impl = (tcpConnectionImpl*)this;
		impl->s.send(buffer, size);
	}

	void tcpConnectionClass::write(const memoryBuffer &buffer)
	{
		write(buffer.data(), buffer.size());
	}

	bool tcpConnectionClass::readLine(string &line)
	{
		available();
		tcpConnectionImpl *impl = (tcpConnectionImpl*)this;
		if (impl->buffer.empty())
			return false;
		char *ptr = &impl->buffer[0], *s = ptr, *e = ptr + impl->buffer.size();
		while (ptr != e)
		{
			if (*ptr == '\n')
			{
				line = string(s, numeric_cast<uint32>(ptr - s));
				detail::memmove(s, ptr + 1, e - ptr - 1);
				impl->buffer.resize(e - ptr - 1);
				return true;
			}
			ptr++;
		}
		return false;
	}

	void tcpConnectionClass::writeLine(const string &str)
	{
		string tmp = str + "\n";
		write((void*)tmp.c_str(), tmp.length());
	}

	uint16 tcpServerClass::port() const
	{
		tcpServerImpl *impl = (tcpServerImpl*)this;
		string a;
		uint16 p;
		impl->socks[0].getLocalAddress().translate(a, p);
		return p;
	}

	holder<tcpConnectionClass> tcpServerClass::accept()
	{
		tcpServerImpl *impl = (tcpServerImpl*)this;

		for (sock &ss : impl->socks)
		{
			try
			{
				sock s = ss.accept();
				if (s.isValid())
					return detail::systemArena().createImpl<tcpConnectionClass, tcpConnectionImpl>(templates::move(s));
			}
			catch (const cage::exception &)
			{
				// do nothing
			}
		}

		return holder<tcpConnectionClass>();
	}

	holder<tcpConnectionClass> newTcpConnection(const string &address, uint16 port)
	{
		return detail::systemArena().createImpl<tcpConnectionClass, tcpConnectionImpl>(address, port);
	}

	holder<tcpServerClass> newTcpServer(uint16 port)
	{
		return detail::systemArena().createImpl<tcpServerClass, tcpServerImpl>(port);
	}
}
