#ifndef guard_systemInformation_h_dsgdfhtdhsdirgrdht54fd54hj54jz
#define guard_systemInformation_h_dsgdfhtdhsdirgrdht54fd54hj54jz

#include "core.h"

namespace cage
{
	CAGE_CORE_API string systemName(); // operating system information
	CAGE_CORE_API string userName(); // user name running this process
	CAGE_CORE_API string hostName(); // name of this computer

	CAGE_CORE_API uint32 processorsCount(); // returns number of threads, that can physically run simultaneously
	CAGE_CORE_API string processorName();
	CAGE_CORE_API uint64 processorClockSpeed(); // Hz

	CAGE_CORE_API uint64 memoryCapacity(); // total memory in bytes for use by the operating system
	CAGE_CORE_API uint64 memoryAvailable(); // estimated unused memory in bytes
}

#endif // guard_systemInformation_h_dsgdfhtdhsdirgrdht54fd54hj54jz
