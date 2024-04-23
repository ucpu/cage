#ifndef guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_
#define guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_

#include <cage-core/files.h>

namespace cage
{
	struct CAGE_CORE_API ProcessPipeEof : public Exception
	{
		using Exception::Exception;
	};

	struct CAGE_CORE_API ProcessCreateConfig
	{
		String command;
		String workingDirectory;

		bool discardStdIn = false;
		bool discardStdOut = false;
		bool discardStdErr = false;

		bool lowerPriority = false;

		ProcessCreateConfig(const String &command, const String &workingDirectory = "");
	};

	class CAGE_CORE_API Process : public File
	{
	public:
		void requestTerminate(); // SIGTERM
		void terminate(); // SIGKILL
		sint32 wait();
	};

	CAGE_CORE_API Holder<Process> newProcess(const ProcessCreateConfig &config);

	CAGE_CORE_API void installSigTermHandler(Delegate<void()> handler);
}

#endif // guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_
