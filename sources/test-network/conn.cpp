#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/network.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/random.h>
#include <cage-core/config.h>
#include <cage-core/variableSmoothingBuffer.h>

using namespace cage;

#include "conn.h"

namespace
{
	configUint32 confMessages("messages");

	class connImpl : public connClass
	{
	public:
		holder<udpConnectionClass> udp;
		memoryBuffer b;
		const uint64 timeStart;
		uint64 timeStats;
		uint64 sendSeqn, recvSeqn, recvCnt, recvBytes;
		variableSmoothingBufferStruct<uint64, 100> smoothRtt;
		variableSmoothingBufferStruct<uint64, 100> smoothThroughput;

		connImpl(holder<udpConnectionClass> udp) : udp(templates::move(udp)), timeStart(getApplicationTime()), timeStats(timeStart + 1000000), sendSeqn(0), recvSeqn(0), recvCnt(0), recvBytes(0)
		{}

		~connImpl()
		{
			statistics(getApplicationTime());
		}

		void statistics(uint64 t)
		{
			uint64 throughput1 = numeric_cast<uint64>(1000000.0 * recvBytes / (t - timeStart));
			uint64 throughput2 = smoothThroughput.smooth();
			uint64 rtt = smoothRtt.smooth();
			double lost = 1.0 - (double)recvCnt / (double)recvSeqn;
			double overhead = 1.0 - (double)recvBytes / (double)udp->statistics().bytesReceivedTotal;
			CAGE_LOG(severityEnum::Info, "conn", string() + "received: " + (recvBytes / 1024) + " KB, messages: " + recvCnt + ", lost: " + lost + ", overhead: " + overhead + ", throughput total: " + (throughput1 / 1024) + " KB/s, smooth: " + (throughput2 / 1024) + " KB/s, rtt: " + (rtt / 1000) + " ms");
		}

		bool process()
		{
			{
				uint64 t = getApplicationTime();
				if (t > timeStats)
				{
					statistics(t);
					timeStats = t + 1000000;
				}
			}
			try
			{
				udp->update();

				{ // read
					while (udp->available())
					{
						memoryBuffer b = udp->read();
						recvBytes += b.size();
						deserializer d(b);
						uint64 r;
						d >> r;
						recvSeqn = max(r, recvSeqn);
						recvCnt++;
					}
				}

				uint32 cnt = confMessages;
				for (uint32 j = 0; j < cnt; j++)
				{ // send
					b.resize(0);
					serializer s(b);
					s << ++sendSeqn;
					uint32 bytes = randomRange(10, 2000);
					if (randomChance() < 0.01)
						bytes *= randomRange(10, 20);
					bytes /= sizeof(decltype(currentRandomGenerator().next()));
					for (uint32 i = 0; i < bytes; i++)
						s << currentRandomGenerator().next();
					udp->write(b, randomRange(0, 20), randomChance() < 0.1);
				}

				{ // statistics
					const auto &s = udp->statistics();
					if (s.roundTripDuration)
					{
						smoothRtt.add(s.roundTripDuration);
						smoothThroughput.add(1000000 * s.bytesDeliveredLately / s.roundTripDuration);
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

bool connClass::process()
{
	connImpl *impl = (connImpl *)this;
	return impl->process();
}

holder<connClass> newConn(holder<udpConnectionClass> udp)
{
	return detail::systemArena().createImpl<connClass, connImpl>(templates::move(udp));
}
