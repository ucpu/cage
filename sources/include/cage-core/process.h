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
		sint32 priority = 0; // 0 = normal, -1 = lower priority, 1 = higher priority
		bool discardStdIn = false;
		bool discardStdOut = false;
		bool discardStdErr = false;
		bool detached = false;

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
	CAGE_CORE_API void installSigIntHandler(Delegate<void()> handler);

	CAGE_CORE_API void openUrl(const String &url);
}

#endif // guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_
