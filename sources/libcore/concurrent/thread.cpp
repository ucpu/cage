#include <set>
#include <exception>

#define CAGE_EXPORT
#include <cage-core/core.h>
#include <cage-core/concurrent.h>
#include <cage-core/log.h>

#ifdef CAGE_SYSTEM_WINDOWS
#include "../incWin.h"
#else
#include <pthread.h>
#endif

#ifdef CAGE_SYSTEM_LINUX
#include <sys/prctl.h>
#endif

#include <cerrno>

namespace cage
{
	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		DWORD WINAPI threadFunctionImpl(LPVOID params);
#else
		void *threadFunctionImpl(void *);
#endif

		class threadImpl : public threadClass
		{
		public:
			const string threadName;
			delegate<void()> function;
			std::exception_ptr exptr;
			uint64 myid;

#ifdef CAGE_SYSTEM_WINDOWS
			HANDLE handle;
#else
			pthread_t handle;
#endif

			threadImpl(delegate<void()> function, const string &threadName) : threadName(threadName), function(function), myid(-1)
			{
#ifdef CAGE_SYSTEM_WINDOWS

				DWORD tid = -1;
				handle = CreateThread(nullptr, 0, &threadFunctionImpl, this, 0, &tid);
				if (!handle)
					CAGE_THROW_ERROR(exception, "CreateThread");
				myid = numeric_cast<uint64>(tid);

#else

				if (pthread_create(&handle, nullptr, &threadFunctionImpl, this) != 0)
					CAGE_THROW_ERROR(codeException, "pthread_create", errno);
				myid = numeric_cast<uint64>(handle);

#endif
			}

			~threadImpl()
			{
				try
				{
					wait();
				}
				catch (...)
				{
					// do nothing
				}

#ifdef CAGE_SYSTEM_WINDOWS
				CloseHandle(handle);
#endif
			}
		};
	}

	uint64 threadClass::id() const
	{
		threadImpl *impl = (threadImpl*)this;
		return impl->myid;
	}

	bool threadClass::done() const
	{
		threadImpl *impl = (threadImpl*)this;

#ifdef CAGE_SYSTEM_WINDOWS
		return WaitForSingleObject(impl->handle, 0) == WAIT_OBJECT_0;
#else
		if (impl->handle == (pthread_t)nullptr)
			return true;
#ifdef CAGE_SYSTEM_MAC
        // mac does not have pthread_tryjoin_np
        switch (pthread_kill(impl->handle, 0))
        {
        case 0: return false;
        default: const_cast<threadClass*>(this)->wait(); return true;
        }
#else
		switch (int err = pthread_tryjoin_np(impl->handle, nullptr))
		{
		case 0: impl->handle = (pthread_t)nullptr; return true;
		case EBUSY: return false;
		default: CAGE_THROW_ERROR(codeException, "pthread_tryjoin_np", err);
		}
#endif
#endif
	}

	void threadClass::wait()
	{
		threadImpl *impl = (threadImpl*)this;

#ifdef CAGE_SYSTEM_WINDOWS
		WaitForSingleObject(impl->handle, INFINITE);
#else
		if (impl->handle != (pthread_t)nullptr)
		{
			switch (int err = pthread_join(impl->handle, nullptr))
			{
			case 0: impl->handle = (pthread_t)nullptr; break;
			default: CAGE_THROW_ERROR(codeException, "pthread_join", err);
			}
		}
#endif

		if (impl->exptr)
		{
			std::exception_ptr tmp;
			std::swap(tmp, impl->exptr);
			std::rethrow_exception(tmp);
		}
	}

	holder<threadClass> newThread(delegate<void()> func, const string &threadName)
	{
		return detail::systemArena().createImpl<threadClass, threadImpl>(func, threadName);
	}

	namespace
	{
		string &currentThreadName()
		{
			static thread_local string name;
			return name;
		}
	}

	void setCurrentThreadName(const string &name)
	{
		string oldName = currentThreadName();
		currentThreadName() = name;
		CAGE_LOG(severityEnum::Info, "thread", string() + "renaming thread id '" + threadId() + "' from '" + oldName + "' to '" + name + "'");

		if (!name.empty())
		{
#ifdef CAGE_SYSTEM_WINDOWS
			static const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
			struct THREADNAME_INFO
			{
				DWORD dwType; // Must be 0x1000.
				LPCSTR szName; // Pointer to name (in user addr space).
				DWORD dwThreadID; // Thread ID (-1=caller thread).
				DWORD dwFlags; // Reserved for future use, must be zero.
			};
#pragma pack(pop)
			THREADNAME_INFO info;
			info.dwType = 0x1000;
			info.szName = name.c_str();
			info.dwThreadID = -1;
			info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
			__try {
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
			}
#pragma warning(pop)
#endif

#ifdef CAGE_SYSTEM_LINUX
			prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
#endif
		}
	}

	string getCurrentThreadName()
	{
		string n = currentThreadName();
		if (n.empty())
			return threadId();
		else
			return n;
	}

	namespace
	{
#ifdef CAGE_SYSTEM_WINDOWS
		DWORD WINAPI threadFunctionImpl(LPVOID params)
#else
		void *threadFunctionImpl(void *params)
#endif
		{
			threadImpl *impl = (threadImpl*)params;
			setCurrentThreadName(impl->threadName);
			//CAGE_LOG(severityEnum::Info, "thread", string() + "thread '" + getCurrentThreadName() + "' started");
			try
			{
				impl->function();
			}
			catch (...)
			{
				impl->exptr = std::current_exception();
				CAGE_LOG(severityEnum::Error, "thread", string() + "unhandled exception in thread '" + getCurrentThreadName() + "'");
			}
			CAGE_LOG(severityEnum::Info, "thread", string() + "thread '" + getCurrentThreadName() + "' ended");
#ifdef CAGE_SYSTEM_WINDOWS
			return 0;
#else
			return nullptr;
#endif
		}
	}
}

