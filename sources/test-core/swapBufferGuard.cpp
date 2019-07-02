#include "main.h"
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/swapBufferGuard.h>
#include <cage-core/threadPool.h>

namespace
{
	template <uint32 BuffersCount>
	class swapBufferTester
	{
	public:
		static const uint32 elementsCount = 100;
		holder<swapBufferGuard> controller;
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

		swapBufferTester() : readIndex(0), writeIndex(0), written(0), read(0), skipped(0), reused(0), generated(0), tested(0), running(false)
		{}

		void run(bool repeatedReads, bool repeatedWrites)
		{
			CAGE_TESTCASE(string() + "buffers count: " + BuffersCount + ", repeated reads: " + repeatedReads + ", repeated writes: " + repeatedWrites);
			running = true;
			swapBufferGuardCreateConfig cfg(BuffersCount);
			cfg.repeatedReads = repeatedReads;
			cfg.repeatedWrites = repeatedWrites;
			controller = newSwapBufferGuard(cfg);
			holder<thread> t1 = newThread(delegate<void()>().bind<swapBufferTester, &swapBufferTester::consumer>(this), "consumer");
			holder<thread> t2 = newThread(delegate<void()>().bind<swapBufferTester, &swapBufferTester::producer>(this), "producer");
			while (read < 2000)
				threadSleep(1000);
			running = false;
			t1->wait();
			t2->wait();
			CAGE_LOG(severityEnum::Info, "swapBufferController", string() + "generated: " + generated + ", written: " + written + ", skipped: " + skipped);
			CAGE_LOG(severityEnum::Info, "swapBufferController", string() + "read: " + read + ", reused: " + reused + ", tested: " + tested);
		}
	};
}

void testSwapBufferGuard()
{
	CAGE_TESTCASE("swapBufferGuard");
	swapBufferTester<2>().run(false, false);
	swapBufferTester<3>().run(false, false);
	swapBufferTester<3>().run(true, false);
	swapBufferTester<3>().run(false, true);
	swapBufferTester<4>().run(false, false);
	swapBufferTester<4>().run(true, false);
	swapBufferTester<4>().run(false, true);
	swapBufferTester<4>().run(true, true);
}
