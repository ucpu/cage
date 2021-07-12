#ifndef guard_tasks_h_vbnmf15dvt89zh
#define guard_tasks_h_vbnmf15dvt89zh

#include "core.h"

namespace cage
{
	namespace privat
	{
		using WaitRunner = void (*)(const struct WaitTask &task, uint32 idx);

		struct CAGE_CORE_API WaitTask : private Noncopyable
		{
			Delegate<void()> function;
			WaitRunner runner = nullptr;
			void *data = nullptr;
			uint32 count = 0;
			sint32 priority = 0;
		};

		CAGE_CORE_API void tasksRun(const WaitTask &task);
	}

	// invoke the function once for each element of the range
	template<class T>
	void tasksRun(Delegate<void(T &)> function, PointerRange<T> data, sint32 priority = 0)
	{
		static_assert(sizeof(privat::WaitTask::function) == sizeof(function));
		privat::WaitTask tsk;
		tsk.function = *(Delegate<void()> *)&function;
		tsk.runner = +[](const privat::WaitTask &task, uint32 idx) {
			Delegate<void(T &)> function = *(Delegate<void(T &)> *)&task.function;
			function(((T *)task.data)[idx]);
		};
		tsk.data = (void *)data.data();
		tsk.count = numeric_cast<uint32>(data.size());
		tsk.priority = priority;
		privat::tasksRun(tsk);
	}

	// invoke operator()() once for each element of the range
	template<class T>
	void tasksRun(PointerRange<T> data, sint32 priority = 0)
	{
		privat::WaitTask tsk;
		tsk.runner = +[](const privat::WaitTask &task, uint32 idx) {
			T &data = ((T *)task.data)[idx];
			data();
		};
		tsk.data = (void *)data.data();
		tsk.count = numeric_cast<uint32>(data.size());
		tsk.priority = priority;
		privat::tasksRun(tsk);
	}

	// invoke the function invocations time, each time with the same data
	template<class T>
	void tasksRun(Delegate<void(T &, uint32)> function, T &data, uint32 invocations, sint32 priority = 0)
	{
		static_assert(sizeof(privat::WaitTask::function) == sizeof(function));
		privat::WaitTask tsk;
		tsk.function = *(Delegate<void()> *)&function;
		tsk.runner = +[](const privat::WaitTask &task, uint32 idx) {
			Delegate<void(T &, uint32)> function = *(Delegate<void(T &, uint32)> *)&task.function;
			function(*(T *)task.data, idx);
		};
		tsk.data = (void *)&data;
		tsk.count = invocations;
		tsk.priority = priority;
		privat::tasksRun(tsk);
	}

	// invoke operator()(uint32) invocations time, each time with the same data
	template<class T>
	void tasksRun(T &data, uint32 invocations, sint32 priority = 0)
	{
		privat::WaitTask tsk;
		tsk.runner = +[](const privat::WaitTask &task, uint32 idx) {
			T &data = *(T *)task.data;
			data(idx);
		};
		tsk.data = (void *)&data;
		tsk.count = invocations;
		tsk.priority = priority;
		privat::tasksRun(tsk);
	}

	// invoke the function invocations time
	CAGE_CORE_API void tasksRun(Delegate<void(uint32)> function, uint32 invocations, sint32 priority = 0);

	namespace detail
	{
		struct CAGE_CORE_API AsyncTask : private Immovable
		{
			void wait();
		};
	}

	namespace privat
	{
		using AsyncRunner = void (*)(const struct AsyncTask &task, uint32 idx);

		struct CAGE_CORE_API AsyncTask : private Noncopyable
		{
			Holder<void> data;
			Delegate<void()> function;
			AsyncRunner runner = nullptr;
			uint32 count = 0;
			sint32 priority = 0;
		};

		CAGE_CORE_API Holder<detail::AsyncTask> tasksRun(AsyncTask &&task);
	}

	// invoke the function once for each element of the range
	template<class T>
	Holder<detail::AsyncTask> tasksRunAsync(Delegate<void(T &)> function, Holder<PointerRange<T>> data, sint32 priority = 0)
	{
		static_assert(sizeof(privat::AsyncTask::function) == sizeof(function));
		privat::AsyncTask tsk;
		tsk.function = *(Delegate<void()> *) & function;
		tsk.runner = +[](const privat::AsyncTask &task, uint32 idx) {
			Delegate<void(T &)> function = *(Delegate<void(T &)> *)&task.function;
			Holder<PointerRange<T>> data = task.data.share().cast<PointerRange<T>>();
			function(data[idx]);
		};
		tsk.count = numeric_cast<uint32>(data.size());
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRun(std::move(tsk));
	}

	// invoke operator()() once for each element of the range
	template<class T>
	Holder<detail::AsyncTask> tasksRunAsync(Holder<PointerRange<T>> data, sint32 priority = 0)
	{
		privat::AsyncTask tsk;
		tsk.runner = +[](const privat::AsyncTask &task, uint32 idx) {
			Holder<PointerRange<T>> data = task.data.share().cast<PointerRange<T>>();
			data[idx]();
		};
		tsk.count = numeric_cast<uint32>(data.size());
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRun(std::move(tsk));
	}

	// invoke the function invocations time, each time with the same data
	template<class T>
	Holder<detail::AsyncTask> tasksRunAsync(Delegate<void(T &, uint32)> function, Holder<T> data, uint32 invocations = 1, sint32 priority = 0)
	{
		static_assert(sizeof(privat::AsyncTask::function) == sizeof(function));
		privat::AsyncTask tsk;
		tsk.function = *(Delegate<void()> *) & function;
		tsk.runner = +[](const privat::AsyncTask &task, uint32 idx) {
			Delegate<void(T &, uint32)> function = *(Delegate<void(T &, uint32)> *)&task.function;
			Holder<T> data = task.data.share().cast<T>();
			function(*data, idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRun(std::move(tsk));
	}

	// invoke operator()(uint32) invocations time, each time with the same data
	template<class T>
	Holder<detail::AsyncTask> tasksRunAsync(Holder<T> data, uint32 invocations = 1, sint32 priority = 0)
	{
		privat::AsyncTask tsk;
		tsk.runner = +[](const privat::AsyncTask &task, uint32 idx) {
			Holder<T> data = task.data.share().cast<T>();
			(*data)(idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRun(std::move(tsk));
	}

	// invoke the function invocations time
	CAGE_CORE_API Holder<detail::AsyncTask> tasksRunAsync(Delegate<void(uint32)> function, uint32 invocations = 1, sint32 priority = 0);

	CAGE_CORE_API std::pair<uint32, uint32> tasksSplit(uint32 groupIndex, uint32 groupsCount, uint32 tasksCount);
}

#endif // guard_tasks_h_vbnmf15dvt89zh
