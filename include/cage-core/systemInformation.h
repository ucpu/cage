#ifndef guard_systemInformation_h_dsgdfhtdhsdirgrdht54fd54hj54jz
#define guard_systemInformation_h_dsgdfhtdhsdirgrdht54fd54hj54jz

namespace cage
{
	CAGE_API string systemName(); // operating system information
	CAGE_API string userName(); // user name running this process
	CAGE_API string hostName(); // name of this computer

	CAGE_API uint32 processorsCount(); // return count of threads, that can physically run simultaneously
	CAGE_API string processorName();
	CAGE_API uint64 processorClockSpeed(); // Hz

	CAGE_API uint64 memoryCapacity(); // total memory in bytes for use by the operating system
	CAGE_API uint64 memoryAvailable(); // unused memory in bytes
}

#endif // guard_systemInformation_h_dsgdfhtdhsdirgrdht54fd54hj54jz
