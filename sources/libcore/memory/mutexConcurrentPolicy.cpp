#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/concurrent.h>

namespace cage
{
	memoryConcurrentPolicyMutex::memoryConcurrentPolicyMutex()
	{
		mutex = newMutex().cast<void>();
	}

	void memoryConcurrentPolicyMutex::lock()
	{
		((mutexClass*)mutex.get())->lock();
	}

	void memoryConcurrentPolicyMutex::unlock()
	{
		((mutexClass*)mutex.get())->unlock();
	}
}