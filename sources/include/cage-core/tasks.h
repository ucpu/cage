#ifndef guard_tasks_h_vbnmf15dvt89zh
#define guard_tasks_h_vbnmf15dvt89zh

#include "core.h"

namespace cage
{
	struct CAGE_CORE_API AsyncTask : private Immovable
	{
		bool done() const;
		void wait();
	};

	namespace privat
	{
		using TaskRunner = void (*)(const struct TaskCreateConfig &task, uint32 idx);

		struct CAGE_CORE_API TaskCreateConfig : private Noncopyable
		{
			StringLiteral name;
			Holder<void> data;
			Delegate<void()> function;
			TaskRunner runner = nullptr;
			uint32 count = 0; // number of invocations
			uint32 size = 0; // number of elements
			sint32 priority = 0;
		};

		CAGE_CORE_API void tasksRunBlocking(TaskCreateConfig &&task);
		CAGE_CORE_API Holder<AsyncTask> tasksRunAsync(TaskCreateConfig &&task);

		template<uint32 Aggregation>
		CAGE_FORCE_INLINE std::pair<uint32, uint32> aggRange(uint32 idx, uint32 size)
		{
			const uint32 b = idx * Aggregation;
			CAGE_ASSERT(b < size);
			uint32 e = b + Aggregation;
			if (e > size)
				e = size;
			return { b, e };
		}

		template<uint32 Aggregation>
		CAGE_FORCE_INLINE uint32 aggCount(uint32 size)
		{
			return size / Aggregation + !!(size % Aggregation);
		}
	}

	// invoke the function once for each element of the range
	template<class T, uint32 Aggregation = 1>
	void tasksRunBlocking(StringLiteral name, Delegate<void(T &)> function, PointerRange<T> data, sint32 priority = 0)
	{
		static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.function = *(Delegate<void()> *)&function;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Delegate<void(T &)> function = *(Delegate<void(T &)> *)&task.function;
			Holder<PointerRange<T>> data = task.data.share().cast<PointerRange<T>>();
			const auto range = privat::aggRange<Aggregation>(idx, task.size);
			for (uint32 i = range.first; i < range.second; i++)
				function(data[i]);
		};
		tsk.data = Holder<PointerRange<T>>(&data, nullptr).template cast<void>();
		tsk.size = numeric_cast<uint32>(data.size());
		tsk.count = privat::aggCount<Aggregation>(tsk.size);
		tsk.priority = priority;
		privat::tasksRunBlocking(std::move(tsk));
	}

	// invoke operator()() once for each element of the range
	template<class T, uint32 Aggregation = 1>
	void tasksRunBlocking(StringLiteral name, PointerRange<T> data, sint32 priority = 0)
	{
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Holder<PointerRange<T>> data = task.data.share().cast<PointerRange<T>>();
			const auto range = privat::aggRange<Aggregation>(idx, task.size);
			for (uint32 i = range.first; i < range.second; i++)
				data[i]();
		};
		tsk.data = Holder<PointerRange<T>>(&data, nullptr).template cast<void>();
		tsk.size = numeric_cast<uint32>(data.size());
		tsk.count = privat::aggCount<Aggregation>(tsk.size);
		tsk.priority = priority;
		privat::tasksRunBlocking(std::move(tsk));
	}

	// invoke the function invocations time, each time with the same data
	template<class T>
	void tasksRunBlocking(StringLiteral name, Delegate<void(T &, uint32)> function, T &data, uint32 invocations, sint32 priority = 0)
	{
		static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.function = *(Delegate<void()> *)&function;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Delegate<void(T &, uint32)> function = *(Delegate<void(T &, uint32)> *)&task.function;
			Holder<T> data = task.data.share().cast<T>();
			function(*data, idx);
		};
		tsk.data = Holder<T>(&data, nullptr).template cast<void>();
		tsk.count = invocations;
		tsk.priority = priority;
		privat::tasksRunBlocking(std::move(tsk));
	}

	// invoke operator()(uint32) invocations time, each time with the same data
	template<class T>
	void tasksRunBlocking(StringLiteral name, T &data, uint32 invocations, sint32 priority = 0)
	{
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Holder<T> data = task.data.share().cast<T>();
			(*data)(idx);
		};
		tsk.data = Holder<T>(&data, nullptr).template cast<void>();
		tsk.count = invocations;
		tsk.priority = priority;
		privat::tasksRunBlocking(std::move(tsk));
	}

	// invoke the function invocations time
	CAGE_CORE_API void tasksRunBlocking(StringLiteral name, Delegate<void(uint32)> function, uint32 invocations, sint32 priority = 0);

	// invoke the function once for each element of the range
	template<class T, uint32 Aggregation = 1>
	Holder<AsyncTask> tasksRunAsync(StringLiteral name, Delegate<void(T &)> function, Holder<PointerRange<T>> data, sint32 priority = 0)
	{
		static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.function = *(Delegate<void()> *) & function;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Delegate<void(T &)> function = *(Delegate<void(T &)> *)&task.function;
			Holder<PointerRange<T>> data = task.data.share().cast<PointerRange<T>>();
			const auto range = privat::aggRange<Aggregation>(idx, task.size);
			for (uint32 i = range.first; i < range.second; i++)
				function(data[i]);
		};
		tsk.size = numeric_cast<uint32>(data.size());
		tsk.count = privat::aggCount<Aggregation>(tsk.size);
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRunAsync(std::move(tsk));
	}

	// invoke operator()() once for each element of the range
	template<class T, uint32 Aggregation = 1>
	Holder<AsyncTask> tasksRunAsync(StringLiteral name, Holder<PointerRange<T>> data, sint32 priority = 0)
	{
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Holder<PointerRange<T>> data = task.data.share().cast<PointerRange<T>>();
			const auto range = privat::aggRange<Aggregation>(idx, task.size);
			for (uint32 i = range.first; i < range.second; i++)
				data[i]();
		};
		tsk.size = numeric_cast<uint32>(data.size());
		tsk.count = privat::aggCount<Aggregation>(tsk.size);
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRunAsync(std::move(tsk));
	}

	// invoke the function invocations time, each time with the same data
	template<class T>
	Holder<AsyncTask> tasksRunAsync(StringLiteral name, Delegate<void(T &, uint32)> function, Holder<T> data, uint32 invocations = 1, sint32 priority = 0)
	{
		static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.function = *(Delegate<void()> *) & function;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Delegate<void(T &, uint32)> function = *(Delegate<void(T &, uint32)> *)&task.function;
			Holder<T> data = task.data.share().cast<T>();
			function(*data, idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRunAsync(std::move(tsk));
	}

	// invoke operator()(uint32) invocations time, each time with the same data
	template<class T>
	Holder<AsyncTask> tasksRunAsync(StringLiteral name, Holder<T> data, uint32 invocations = 1, sint32 priority = 0)
	{
		privat::TaskCreateConfig tsk;
		tsk.name = name;
		tsk.runner = +[](const privat::TaskCreateConfig &task, uint32 idx) {
			Holder<T> data = task.data.share().cast<T>();
			(*data)(idx);
		};
		tsk.count = invocations;
		tsk.priority = priority;
		tsk.data = std::move(data).template cast<void>();
		return privat::tasksRunAsync(std::move(tsk));
	}

	// invoke the function invocations time
	CAGE_CORE_API Holder<AsyncTask> tasksRunAsync(StringLiteral name, Delegate<void(uint32)> function, uint32 invocations = 1, sint32 priority = 0);

	// allows running higher priority tasks from inside long running task
	CAGE_CORE_API void tasksYield();

	CAGE_CORE_API std::pair<uint32, uint32> tasksSplit(uint32 groupIndex, uint32 groupsCount, uint32 tasksCount);
}

#endif // guard_tasks_h_vbnmf15dvt89zh
