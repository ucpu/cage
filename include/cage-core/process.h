#ifndef guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_
#define guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_

namespace cage
{
	class CAGE_API processHandle : private immovable
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

	CAGE_API holder<processHandle> newProcess(const string &cmd);
	CAGE_API holder<processHandle> newProcess(const string &cmd, const string &workingDirectory);
}

#endif // guard_program_h_f16ac3b2_6520_4503_a6ad_f4a582216f67_
