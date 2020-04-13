#ifndef guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_
#define guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API ProcessCreateConfig
	{
		string cmd;
		string workingDirectory;
		// todo bool discardStdIn
		// todo bool discardStdOut
		// todo bool discardStdErr
		// todo option to modify environment

		ProcessCreateConfig(const string &cmd, const string &workingDirectory = "");
	};

	class CAGE_CORE_API Process : private Immovable
	{
	public:
		string getCmdString() const;
		string getWorkingDir() const;

		void read(void *data, uint32 size);
		string readLine();
		void write(const void *data, uint32 size);
		void writeLine(const string &data);

		void terminate();
		int wait();
	};

	CAGE_CORE_API Holder<Process> newProcess(const ProcessCreateConfig &config);
}

#endif // guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_
