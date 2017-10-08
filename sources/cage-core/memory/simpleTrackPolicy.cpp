#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/memory.h>
#include <cage-core/log.h>

namespace cage
{
	memoryTrackPolicySimple::~memoryTrackPolicySimple()
	{
		if (count > 0)
		{
			CAGE_LOG(severityEnum::Critical, "memory", "memory leak detected");
			detail::terminate();
		}
	}
}