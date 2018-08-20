namespace cage
{
	class mutexImpl : public mutexClass
	{
	public:
#ifdef CAGE_SYSTEM_WINDOWS
		CRITICAL_SECTION cs;
#else
		pthread_mutex_t mut;
#endif

		mutexImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			InitializeCriticalSection(&cs);
#else
			pthread_mutex_init(&mut, nullptr);
#endif
		}

		~mutexImpl()
		{
#ifdef CAGE_SYSTEM_WINDOWS
			DeleteCriticalSection(&cs);
#else
			pthread_mutex_destroy(&mut);
#endif
		}
	};
}
