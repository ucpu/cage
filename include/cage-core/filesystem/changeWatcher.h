namespace cage
{
	class CAGE_API changeWatcherClass
	{
	public:
		void registerPath(const string &path);
		string waitForChange(uint64 time = m);
	};

	CAGE_API holder<changeWatcherClass> newChangeWatcher();
}
