#include "main.h"

#include <cage-core/memory.h>
#include <cage-core/utility/commandBucket.h>

namespace
{
	static uint32 counter = 0;

	void updateByOne()
	{
		counter++;
	}

	void updateByArgument(uint32 val)
	{
		counter += val;
	}

	const uint32 value()
	{
		return counter;
	}

	const uint32 updateAndReturn(uint32 a, uint32 b)
	{
		return counter += a + b;
	}

	struct updater
	{
		uint32 val;

		void update()
		{
			counter += val;
		}

		void update(uint32 a)
		{
			counter += a;
		}

		const uint32 updateAndReturn(uint32 a)
		{
			return counter += a;
		}

		const uint32 value() const
		{
			return counter;
		}

		static void updateStatic()
		{
			counter++;
		}

		static const uint32 valueStatic()
		{
			return counter;
		}
	};
}

void testCommandBucket()
{
	CAGE_TESTCASE("command buckets");
	memoryArenaGrowing<memoryAllocatorPolicyLinear<>, memoryConcurrentPolicyNone> arenaA(1024 * 1024 * 4);
	memoryArena arenaB(&arenaA);
	commandBucket bucket(arenaB);

	updater up;
	up.val = 1;

	bucket.addFunction<void, &updateByOne>();
	bucket.addFunction<void, uint32, &updateByArgument>(1);
	bucket.addFunction<const uint32, uint32, uint32, &updateAndReturn>(1, 1);
	bucket.addFunction<const uint32, &value>();
	bucket.addFunction<void, &updateByOne>();
	bucket.addMethod<updater, void, static_cast<void (updater::*)()>(&updater::update)>(&up);
	bucket.addMethod<updater, void, uint32, static_cast<void (updater::*)(uint32)>(&updater::update)>(&up, 1);
	bucket.addMethod<updater, const uint32, uint32, &updater::updateAndReturn>(&up, 1);
	bucket.addConst<updater, const uint32, &updater::value>(&up);
	bucket.addFunction<void, &updater::updateStatic>();
	bucket.addFunction<const uint32, &updater::valueStatic>();
	CAGE_TEST(counter == 0);
	bucket.dispatch();
	CAGE_TEST(counter == 9);
	bucket.flush();
	counter = 0;

	bucket.dispatch();
	CAGE_TEST(counter == 0);

	bucket.addFunction<void, &updateByOne>();
	bucket.addFunction<void, uint32, &updateByArgument>(3);
	bucket.dispatch();
	CAGE_TEST(counter == 4);
	bucket.dispatch();
	CAGE_TEST(counter == 8);
	bucket.flush();
	counter = 0;
}