#include <vector>

#include <cage-core/endianness.h>
#include <cage-core/hashes.h>
#include <cage-core/lineReader.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/networkTcp.h>
#include <cage-core/networkWebsocket.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>

namespace cage
{
	using namespace privat;

	namespace
	{
		class WebsocketConnectionImpl final : public WebsocketConnection
		{
		public:
			Holder<TcpConnection> con;
			std::vector<char> tcp; // data stream read from tcp, contains websocket framing
			std::vector<char> staging; // unmasked data acquired from frames, waiting for FIN
			std::vector<char> working; // data prepared for consumption by the application
			const bool masking = false;

			WebsocketConnectionImpl(const String &address, uint16 port) : masking(true)
			{
				con = newTcpConnection(address, port);
				con->write("GET / HTTP/1.1\r\n");
				con->write(String(Stringizer() + "Sec-WebSocket-Key: " + randomRange(uint32(1), (uint32(m))) + "\r\n"));
				con->write("Connection: keep-alive, Upgrade\r\n");
				con->write("Upgrade: websocket\r\n");
				con->write("\r\n");
				while (true)
				{
					String s = con->readLine();
					//CAGE_LOG_DEBUG(SeverityEnum::Info, "websocket", s);
					if (s.empty())
						break;
				}
			}

			WebsocketConnectionImpl(Holder<TcpConnection> &&con_) : con(std::move(con_))
			{
				String key;
				while (true)
				{
					String s = con->readLine();
					//CAGE_LOG_DEBUG(SeverityEnum::Info, "websocket", s);
					if (s.empty())
						break;
					String a = split(s, ":");
					if (trim(a) == "Sec-WebSocket-Key")
					{
						if (!key.empty())
							CAGE_THROW_ERROR(Exception, "invalid websocket connection request - duplicate security key");
						key = trim(s);
					}
				}
				if (key.empty())
					CAGE_THROW_ERROR(Exception, "invalid websocket connection request - missing security key");
				con->write("HTTP/1.1 101 Switching Protocols\r\n");
				con->write("Upgrade: websocket\r\n");
				con->write("Connection: Upgrade\r\n");
				con->write(String(Stringizer() + "Sec-WebSocket-Accept: " + hashToBase64(hashSha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")) + "\r\n"));
				con->write("\r\n");
			}

			void copyWithMask(PointerRange<char> dst, PointerRange<const char> src, uint32 mask)
			{
				CAGE_ASSERT(dst.size() == src.size());
				const uint8 ms[4] = { uint8(mask), uint8(mask >> 8), uint8(mask >> 16), uint8(mask >> 24) };
				char *d = dst.data();
				uint32 i = 0;
				for (const char s : src)
					*d++ = (uint8)s ^ ms[i++ % 4];
			}

			void update()
			{
				if (!working.empty())
					return;

				{ // read tcp
					const auto s = con->size();
					if (s > 0)
					{
						tcp.resize(tcp.size() + s);
						con->read({ tcp.data() + tcp.size() - s, tcp.data() + tcp.size() });
					}
				}

				while (true)
				{
					if (tcp.size() < 2)
						return;
					Deserializer des(tcp);
					uint8 op = 0, sz1 = 0;
					des >> op >> sz1;
					const bool fin = op & 128;
					op = op & 15;
					const bool masked = sz1 & 128;
					sz1 = sz1 & 127;
					uint64 size = 0;
					if (sz1 < 126)
						size = sz1;
					else if (sz1 == 126)
					{
						if (des.available() < 2)
							return;
						uint16 s = 0;
						des >> s;
						size = endianness::change(s);
					}
					else
					{
						CAGE_ASSERT(sz1 == 127);
						if (des.available() < 8)
							return;
						des >> size;
						size = endianness::change(size);
					}
					if (des.available() < size)
						return;

					if (op < 3)
					{
						staging.resize(staging.size() + size);
						PointerRange<char> dst = { staging.data() + staging.size() - size, staging.data() + staging.size() };
						if (masked)
						{
							uint32 mask = 0;
							des >> mask;
							copyWithMask(dst, des.read(size), mask);
						}
						else
							detail::memcpy(dst.data(), des.read(size).data(), size);
					}
					else
						des.read(size);

					const uintPtr erased = tcp.size() - des.available();
					tcp.erase(tcp.begin(), tcp.begin() + erased);

					if (fin)
					{
						std::swap(staging, working);
						return;
					}
				}
			}

			void read(PointerRange<char> data) override
			{
				update();
				if (data.size() > working.size())
					CAGE_THROW_ERROR(Exception, "insufficient data in websocket stream");
				detail::memcpy(data.data(), working.data(), data.size());
				working.erase(working.begin(), working.begin() + data.size());
			}

			void write(PointerRange<const char> data) override
			{
				MemoryBuffer buff;
				buff.reserve(data.size() + 20);
				Serializer ser(buff);
				{
					uint8 c = 0;
					c |= 128; // fin
					// rsv ignored
					c |= 2; // opcode
					ser << c;
				}
				{
					const uint8 mb = masking ? 128 : 0;
					if (data.size() < 126)
						ser << uint8(data.size() | mb);
					else if (data.size() < 65536)
						ser << uint8(126 | mb) << endianness::change(uint16(data.size()));
					else
						ser << uint8(127 | mb) << endianness::change(uint64(data.size()));
				}
				if (masking)
				{
					const uint32 mask = randomRange(uint32(1), uint32(m));
					ser << mask;
					copyWithMask(ser.write(data.size()), data, mask);
				}
				else
					ser.write(data);
				con->write(buff);
			}

			uint64 size() override
			{
				update();
				return working.size();
			}

			bool readLine(String &line) override
			{
				update();
				const auto s = detail::readLine(line, working, true);
				if (s > 0)
					working.erase(working.begin(), working.begin() + s);
				return s > 0;
			}
		};

		class WebsocketServerImpl : public WebsocketServer
		{
		public:
			Holder<TcpServer> srv;

			WebsocketServerImpl(uint16 port) { srv = newTcpServer(port); }
		};
	}

	TcpRemoteInfo WebsocketConnection::remoteInfo() const
	{
		const WebsocketConnectionImpl *impl = (const WebsocketConnectionImpl *)this;
		return impl->con->remoteInfo();
	}

	uint16 WebsocketServer::port() const
	{
		const WebsocketServerImpl *impl = (const WebsocketServerImpl *)this;
		return impl->srv->port();
	}

	Holder<WebsocketConnection> WebsocketServer::accept()
	{
		WebsocketServerImpl *impl = (WebsocketServerImpl *)this;
		Holder<TcpConnection> c = impl->srv->accept();
		if (c)
		{
			try
			{
				return systemMemory().createImpl<WebsocketConnection, WebsocketConnectionImpl>(std::move(c));
			}
			catch (const cage::Exception &)
			{
				// do nothing
			}
		}
		return Holder<WebsocketConnection>();
	}

	Holder<WebsocketConnection> newWebsocketConnection(const String &address, uint16 port)
	{
		return systemMemory().createImpl<WebsocketConnection, WebsocketConnectionImpl>(address, port);
	}

	Holder<WebsocketServer> newWebsocketServer(uint16 port)
	{
		return systemMemory().createImpl<WebsocketServer, WebsocketServerImpl>(port);
	}
}
