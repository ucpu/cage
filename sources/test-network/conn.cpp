#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/network.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/serialization.h>

using namespace cage;

#include "conn.h"

namespace
{
	class connImpl : public connClass
	{
	public:
		holder<udpConnectionClass> udp;
		memoryBuffer b;
		const uint64 timeStart;
		uint64 bytes;
		uint32 sent, received;
		uint32 step;

		connImpl(holder<udpConnectionClass> udp) : udp(templates::move(udp)), timeStart(getApplicationTime()), bytes(0), sent(0), received(0), step(0)
		{}

		~connImpl()
		{
			statistics();
		}

		void statistics()
		{
			uint64 throughput = numeric_cast<uint64>(1000000.0 * bytes / (getApplicationTime() - timeStart));
			CAGE_LOG(severityEnum::Info, "conn", string() + "messages: sent: " + sent + ", received: " + received + ", ratio: " + (100.f * received / sent) + " %");
			CAGE_LOG(severityEnum::Info, "conn", string() + "data: received: " + (bytes / 1024) + " KB, throughput: " + (throughput / 1024) + " KB/s");
		}

		bool process()
		{
			if ((step % 30) == 15)
				statistics();
			if (step++ > 5 * 60 * 30)
			{
				CAGE_LOG(severityEnum::Info, "conn", "consider me done");
				return true;
			}
			try
			{
				udp->update();

				{ // read
					while (udp->available())
					{
						received++;
						memoryBuffer b = udp->read();
						bytes += b.size();
					}
				}

				for (uint32 j = 0; j < 10; j++)
				{ // send
					b.resize(0);
					serializer s(b);
					uint32 cnt = randomRange(5, 1000);
					if (randomChance() < 0.1)
						cnt *= 100;
					for (uint32 i = 0; i < cnt; i++)
						s << (char)randomRange(0, 255);
					udp->write(b, randomRange(0, 100), randomChance() < 0.1);
					sent++;
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
