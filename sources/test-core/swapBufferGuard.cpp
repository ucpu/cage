#include <atomic>

#include "main.h"

#include <cage-core/concurrent.h>
#include <cage-core/math.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/threadPool.h>

namespace
{
	template<uint32 BuffersCount>
	class SwapBufferTester
	{
	public:
		static constexpr uint32 ElementsCount = 100;
		Holder<SwapBufferGuard> controller;
		uint32 buffers[BuffersCount][ElementsCount] = {};
		uint32 sums[BuffersCount] = {};
		uint32 indices[BuffersCount] = {};
		std::atomic<uint32> readIndex = 0, writeIndex = 0;
		std::atomic<uint32> written = 0, read = 0, skipped = 0, reused = 0, generated = 0, tested = 0;
		std::atomic<bool> running = true;

		void consumer()
		{
			uint32 tmpBuff[ElementsCount];
			detail::memset(tmpBuff, 0, sizeof(tmpBuff));
			uint32 tmpSum = 0;
			while (running)
			{
				if (auto mark = controller->read())
				{
					for (uint32 i = 0; i < ElementsCount; i++)
						tmpBuff[i] = buffers[mark.index()][i];
					tmpSum = sums[mark.index()];
					CAGE_TEST(indices[mark.index()] >= readIndex);
					readIndex = indices[mark.index()];
					read++;
				}
				else
					reused++;
				threadYield();
				{ // consume
					uint32 sum = 0;
					for (uint32 i = 0; i < ElementsCount; i++)
						sum += tmpBuff[i];
					CAGE_TEST(sum == tmpSum);
					tested++;
				}
			}
		}

		void producer()
		{
			uint32 tmpBuff[ElementsCount];
			detail::memset(tmpBuff, 0, sizeof(tmpBuff));
			uint32 tmpSum = 0;
			while (running)
			{
				{ // produce
					uint32 sum = 0;
					for (uint32 i = 0; i < ElementsCount; i++)
						sum += (tmpBuff[i] = randomRange(0u, m));
					tmpSum = sum;
					generated++;
				}
				threadYield();
				if (auto mark = controller->write())
				{
					for (uint32 i = 0; i < ElementsCount; i++)
						buffers[mark.index()][i] = tmpBuff[i];
					sums[mark.index()] = tmpSum;
					indices[mark.index()] = writeIndex++;
					written++;
				}
				else
					skipped++;
			}
		}

		void run(bool repeatedReads, bool repeatedWrites)
		{
			CAGE_TESTCASE(Stringizer() + "buffers count: " + BuffersCount + ", repeated reads: " + repeatedReads + ", repeated writes: " + repeatedWrites);
			SwapBufferGuardCreateConfig cfg;
			cfg.buffersCount = BuffersCount;
			cfg.repeatedReads = repeatedReads;
			cfg.repeatedWrites = repeatedWrites;
			controller = newSwapBufferGuard(cfg);
			{
				Holder<Thread> t1 = newThread(Delegate<void()>().bind<SwapBufferTester, &SwapBufferTester::consumer>(this), "consumer");
				Holder<Thread> t2 = newThread(Delegate<void()>().bind<SwapBufferTester, &SwapBufferTester::producer>(this), "producer");
				while (read < 2000)
					threadSleep(1000);
				running = false;
			}
			CAGE_LOG(SeverityEnum::Info, "swapBufferController", Stringizer() + "generated: " + generated + ", written: " + written + ", skipped: " + skipped);
			CAGE_LOG(SeverityEnum::Info, "swapBufferController", Stringizer() + "read: " + read + ", reused: " + reused + ", tested: " + tested);
		}
	};
}

void testSwapBufferGuard()
{
	CAGE_TESTCASE("SwapBufferGuard");
	SwapBufferTester<2>().run(false, false);
	SwapBufferTester<3>().run(false, false);
	SwapBufferTester<3>().run(true, false);
	SwapBufferTester<3>().run(false, true);
	SwapBufferTester<4>().run(false, false);
	SwapBufferTester<4>().run(true, false);
	SwapBufferTester<4>().run(false, true);
	SwapBufferTester<4>().run(true, true);
}
