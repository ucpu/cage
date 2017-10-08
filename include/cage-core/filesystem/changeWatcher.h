namespace cage
{
	class CAGE_API changeWatcherClass
	{
	public:
		void registerPath(const string &path);
		string waitForChange(uint64 time = -1);
	};

	CAGE_API holder<changeWatcherClass> newChangeWatcher();
}
