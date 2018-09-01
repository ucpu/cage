#include "main.h"
#include <cage-core/math.h>
#include <cage-core/concurrent.h>
#include <cage-core/utility/threadSafeDoubleBufferController.h>
#include <cage-core/utility/threadPool.h>

namespace
{
	holder<threadSafeDoubleBufferControllerClass> controller;
	const uint32 elementsCount = 100;
	// 0,1 = controlled buffers, 2 = consummer buffer, 3 = producer buffer
	uint32 buffers[4][elementsCount];
	uint32 sums[4];
	uint32 written, read, skipped, reused, generated, tested;
	bool running;

	void consummer()
	{
		while (running && generated == 0)
			threadSleep(0);
		while (running)
		{
			threadSleep(0);
			if (auto mark = controller->read())
			{
				for (uint32 i = 0; i < elementsCount; i++)
					buffers[2][i] = buffers[mark.index][i];
				sums[2] = sums[mark.index];
				read++;
			}
			else
				reused++;
			{
				uint32 sum = 0;
				for (uint32 i = 0; i < elementsCount; i++)
					sum += buffers[2][i];
				CAGE_TEST(sum == sums[2]);
				tested++;
			}
		}
	}

	void producer()
	{
		while (running)
		{
			threadSleep(0);
			{
				uint32 sum = 0;
				for (uint32 i = 0; i < elementsCount; i++)
					sum += (buffers[3][i] = random(0u, (uint32)-1));
				sums[3] = sum;
				generated++;
			}
			if (auto mark = controller->write())
			{
				for (uint32 i = 0; i < elementsCount; i++)
					buffers[mark.index][i] = buffers[3][i];
				sums[mark.index] = sums[3];
				written++;
			}
			else
				skipped++;
		}
	}
}

void testThreadSafeDoubleBufferController()
{
	CAGE_TESTCASE("threadSafeDoubleBufferController");

	running = true;
	controller = newThreadSafeDoubleBufferController();
	holder<threadClass> t1 = newThread(delegate<void()>().bind<&consummer>(), "consummer");
	holder<threadClass> t2 = newThread(delegate<void()>().bind<&producer>(), "producer");
	while (read < 2000)
		threadSleep(1000);
	running = false;
	t1->wait();
	t2->wait();
	CAGE_LOG(severityEnum::Info, "threadSafeDoubleBufferController", string() + "generated: " + generated + ", written: " + written + ", skipped: " + skipped);
	CAGE_LOG(severityEnum::Info, "threadSafeDoubleBufferController", string() + "read: " + read + ", reused: " + reused + ", tested: " + tested);
}
