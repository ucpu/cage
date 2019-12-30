#include "main.h"
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/threadPool.h>

namespace
{
	template <uint32 BuffersCount>
	class SwapBufferTester
	{
	public:
		static const uint32 elementsCount = 100;
		Holder<SwapBufferGuard> controller;
		uint32 buffers[BuffersCount][elementsCount];
		uint32 sums[BuffersCount];
		uint32 indices[BuffersCount];
		uint32 readIndex, writeIndex;
		uint32 written, read, skipped, reused, generated, tested;
		bool running;

		void consumer()
		{
			uint32 tmpBuff[elementsCount];
			detail::memset(tmpBuff, 0, sizeof(tmpBuff));
			uint32 tmpSum = 0;
			//while (running && generated == 0)
			//	threadSleep(1000);
			while (running)
			{
				if (auto mark = controller->read())
				{
					for (uint32 i = 0; i < elementsCount; i++)
						tmpBuff[i] = buffers[mark.index()][i];
					tmpSum = sums[mark.index()];
					CAGE_TEST(indices[mark.index()] >= readIndex);
					readIndex = indices[mark.index()];
					read++;
				}
				else
					reused++;
				threadSleep(0);
				{ // consume
					uint32 sum = 0;
					for (uint32 i = 0; i < elementsCount; i++)
						sum += tmpBuff[i];
					CAGE_TEST(sum == tmpSum);
					tested++;
				}
			}
		}

		void producer()
		{
			uint32 tmpBuff[elementsCount];
			detail::memset(tmpBuff, 0, sizeof(tmpBuff));
			uint32 tmpSum = 0;
			while (running)
			{
				{ // produce
					uint32 sum = 0;
					for (uint32 i = 0; i < elementsCount; i++)
						sum += (tmpBuff[i] = randomRange(0u, m));
					tmpSum = sum;
					generated++;
				}
				threadSleep(0);
				if (auto mark = controller->write())
				{
					for (uint32 i = 0; i < elementsCount; i++)
						buffers[mark.index()][i] = tmpBuff[i];
					sums[mark.index()] = tmpSum;
					indices[mark.index()] = writeIndex++;
					written++;
				}
				else
					skipped++;
			}
		}

		SwapBufferTester() : readIndex(0), writeIndex(0), written(0), read(0), skipped(0), reused(0), generated(0), tested(0), running(false)
		{}

		void run(bool repeatedReads, bool repeatedWrites)
		{
			CAGE_TESTCASE(stringizer() + "buffers count: " + BuffersCount + ", repeated reads: " + repeatedReads + ", repeated writes: " + repeatedWrites);
			running = true;
			SwapBufferGuardCreateConfig cfg(BuffersCount);
			cfg.repeatedReads = repeatedReads;
			cfg.repeatedWrites = repeatedWrites;
			controller = newSwapBufferGuard(cfg);
			Holder<Thread> t1 = newThread(Delegate<void()>().bind<SwapBufferTester, &SwapBufferTester::consumer>(this), "consumer");
			Holder<Thread> t2 = newThread(Delegate<void()>().bind<SwapBufferTester, &SwapBufferTester::producer>(this), "producer");
			while (read < 2000)
				threadSleep(1000);
			running = false;
			t1->wait();
			t2->wait();
			CAGE_LOG(SeverityEnum::Info, "swapBufferController", stringizer() + "generated: " + generated + ", written: " + written + ", skipped: " + skipped);
			CAGE_LOG(SeverityEnum::Info, "swapBufferController", stringizer() + "read: " + read + ", reused: " + reused + ", tested: " + tested);
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
