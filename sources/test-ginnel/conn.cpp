#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/networkGinnel.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>
#include <cage-core/random.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-core/string.h>
#include <cage-core/config.h>

using namespace cage;

#include "conn.h"

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
		VariableSmoothingBuffer<uint64, 20> smoothRtt;
		VariableSmoothingBuffer<uint64, 20> smoothThroughput;

		ConnImpl(Holder<GinnelConnection> udp) : udp(std::move(udp)), timeStart(applicationTime())
		{
			timeStats = timeStart + 500000;
			lastProcessTime = timeStart;
			maxBytesPerSecond = configGetUint64("maxBytesPerSecond");
			CAGE_LOG(SeverityEnum::Info, "config", Stringizer() + "limit: " + (maxBytesPerSecond / 1024) + " KB/s");
		}

		~ConnImpl()
		{
			statistics(applicationTime());
		}

		template<class T>
		static String leftFill(const T &value, uint32 n = 6)
		{
			String s = Stringizer() + value;
			s = subString(s, 0, n);
			s = reverse(fill(reverse(s), n));
			return s;
		}

		void statistics(uint64 t)
		{
			const uint64 throughput1 = 1000000 * recvBytes / (t - timeStart);
			const double lost = 1.0 - (double)recvCnt / (double)recvSeqn;
			const double overhead = 1.0 - (double)recvBytes / (double)udp->statistics().bytesReceivedTotal;
			CAGE_LOG(SeverityEnum::Info, "conn", Stringizer() + "received: " + leftFill(recvBytes / 1024) + " KB, messages: " + leftFill(recvCnt) + ", lost: " + leftFill(lost) + ", overhead: " + leftFill(overhead) + ", rate: " + leftFill(throughput1 / 1024) + " KB/s, send rate: " + leftFill(smoothThroughput.smooth() / 1024) + " KB/s, estimated bandwidth: " + leftFill(udp->bandwidth() / 1024) + " KB/s, rtt: " + leftFill(smoothRtt.smooth() / 1000) + " ms");
		}

		bool process()
		{
			const uint64 currentTime = applicationTime();
			const uint64 deltaTime = currentTime - lastProcessTime;
			lastProcessTime = currentTime;

			// show statistics
			if (currentTime > timeStats)
			{
				statistics(currentTime);
				timeStats = currentTime + 1000000;
			}

			try
			{
				udp->update();

				{ // read
					while (udp->available())
					{
						Holder<PointerRange<char>> b = udp->read();
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

				{ // update statistics
					const auto &s = udp->statistics();
					if (s.roundTripDuration > 0 && s.timestamp != lastStatsTimestamp)
					{
						lastStatsTimestamp = s.timestamp;
						smoothRtt.add(s.roundTripDuration);
						smoothThroughput.add(s.bpsDelivered());
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
