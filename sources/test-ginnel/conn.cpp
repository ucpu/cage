#include "common.h"

#include <cage-core/config.h>
#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/networkGinnel.h>
#include <cage-core/random.h>
#include <cage-core/serialization.h>
#include <cage-core/string.h>

namespace
{
	class ConnImpl : public Conn
	{
	public:
		Holder<GinnelConnection> udp;
		const uint64 timeStart = 0;
		uint64 timeStats = 0;
		uint64 lastProcessTime = 0;
		uint64 sendSeqn = 0, recvSeqn = 0, recvCnt = 0, recvBytes = 0;
		uint64 lastStatsTimestamp = 0;
		uint64 maxBytesPerSecond = 0;

		ConnImpl(Holder<GinnelConnection> udp) : udp(std::move(udp)), timeStart(applicationTime())
		{
			timeStats = timeStart + 500000;
			lastProcessTime = timeStart;
			maxBytesPerSecond = configGetUint64("maxBytesPerSecond");
			CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "limit: " + (maxBytesPerSecond / 1024) + " KB/s");
		}

		~ConnImpl() { statistics(); }

		template<class T>
		static String leftFill(const T &value, uint32 n = 6)
		{
			String s = Stringizer() + value;
			s = subString(s, 0, n);
			s = reverse(fill(reverse(s), n));
			return s;
		}

		void statistics()
		{
			const auto s = udp->statistics();
			CAGE_LOG(SeverityEnum::Info, "conn", Stringizer() + "receiving: " + leftFill(s.bpsReceived() / 1024) + " KB/s, sending: " + leftFill(s.bpsSent() / 1024) + " KB/s, estimated bandwidth: " + leftFill(s.estimatedBandwidth / 1024) + " KB/s, ping: " + leftFill(s.roundTripDuration / 1000) + " ms");
		}

		bool process()
		{
			const uint64 currentTime = applicationTime();
			const uint64 deltaTime = currentTime - lastProcessTime;
			lastProcessTime = currentTime;

			// show statistics
			if (currentTime > timeStats)
			{
				statistics();
				timeStats = currentTime + 1000000;
			}

			try
			{
				udp->update();

				{ // read
					while (true)
					{
						Holder<PointerRange<char>> b = udp->read();
						if (!b)
							break;
						recvBytes += b.size();
						Deserializer d(b);
						uint64 r;
						d >> r;
						recvSeqn = max(r, recvSeqn);
						recvCnt++;
					}
				}

				{ // send
					uint64 total = 0;
					while (total < deltaTime * maxBytesPerSecond / 1000000)
					{
						uint32 bytes = randomRange(10, 2000);
						if (udp->capacity() < bytes)
							break;
						total += bytes;
						if (randomChance() < 0.001) // spontaneously send much more data to simulate unexpected events
							bytes *= randomRange(10, 20);
						MemoryBuffer b;
						Serializer s(b);
						s << ++sendSeqn;
						while (b.size() < bytes)
							s << detail::randomGenerator().next();
						udp->write(b, randomRange(0, 20), randomChance() < 0.1);
					}
				}

				return false;
			}
			catch (...)
			{
				return true;
			}
		}
	};
}

bool Conn::process()
{
	ConnImpl *impl = (ConnImpl *)this;
	return impl->process();
}

Holder<Conn> newConn(Holder<GinnelConnection> udp)
{
	return systemMemory().createImpl<Conn, ConnImpl>(std::move(udp));
}
