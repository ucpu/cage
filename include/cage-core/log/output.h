namespace cage
{
	CAGE_API void logOutputPolicyDebug(const string &message);
	CAGE_API void logOutputPolicyStdOut(const string &message);
	CAGE_API void logOutputPolicyStdErr(const string &message);

	class CAGE_API logOutputPolicyFileClass
	{
	public:
		void output(const string &message);
	};

	CAGE_API holder<logOutputPolicyFileClass> newLogOutputPolicyFile(const string &path, bool append);
}
