#ifndef guard_tasks_h_vbnmf15dvt89zh
#define guard_tasks_h_vbnmf15dvt89zh

#include <cage-core/core.h>

namespace cage
{
	struct CAGE_CORE_API AsyncTask : private Immovable
	{
		[[nodiscard]] bool done() const;
		void wait();
		void abort();
	};

	namespace privat
	{
		struct CAGE_CORE_API TaskCreateConfig : private Noncopyable
		{
			using TaskRunner = void (*)(const TaskCreateConfig &task, uint32 idx);
			StringPointer name;
			Holder<void> data;
			Delegate<void()> function;
			TaskRunner runner = nullptr;
			uint32 elements = 0;
			uint32 invocations = 0;
		};

		template<uint32 Aggregation>
		CAGE_FORCE_INLINE std::pair<uint32, uint32> aggRange(uint32 idx, uint32 elements)
		{
			static_assert(Aggregation > 1);
			const uint32 b = idx * Aggregation;
			CAGE_ASSERT(b < elements);
			uint32 e = b + Aggregation;
			if (e > elements)
				e = elements;
			return { b, e };
		}

		template<>
		CAGE_FORCE_INLINE std::pair<uint32, uint32> aggRange<1>(uint32 idx, uint32 elements)
		{
			return { idx, idx + 1 };
		}

		template<uint32 Aggregation>
		CAGE_FORCE_INLINE uint32 aggInvocations(uint32 elements)
		{
			static_assert(Aggregation > 1);
			return elements / Aggregation + !!(elements % Aggregation);
		}

		template<>
		CAGE_FORCE_INLINE uint32 aggInvocations<1>(uint32 elements)
		{
			return elements;
		}

		CAGE_CORE_API void tasksRunBlocking(TaskCreateConfig &&task);
		CAGE_CORE_API Holder<AsyncTask> tasksRunAsync(TaskCreateConfig &&task);

		template<bool Async>
		auto tasksRunImpl(TaskCreateConfig &&task);
		template<>
		CAGE_FORCE_INLINE auto tasksRunImpl<false>(TaskCreateConfig &&task)
		{
			return tasksRunBlocking(std::move(task));
		}
		template<>
		CAGE_FORCE_INLINE auto tasksRunImpl<true>(TaskCreateConfig &&task)
		{
			return tasksRunAsync(std::move(task));
		}

		template<class T, uint32 Aggregation, bool Async>
		auto tasksRunImpl(StringPointer name, Delegate<void(T &)> function, Holder<PointerRange<T>> data)
		{
			static_assert(sizeof(TaskCreateConfig::function) == sizeof(function));
			TaskCreateConfig tsk;
			tsk.name = name;
			tsk.function = reinterpret_cast<Delegate<void()> &>(function);
			tsk.runner = +[](const TaskCreateConfig &task, uint32 idx)
			{
				const Delegate<void(T &)> function = reinterpret_cast<const Delegate<void(T &)> &>(task.function);
				const PointerRange<T> &data = *static_cast<PointerRange<T> *>(+task.data);
				const auto range = aggRange<Aggregation>(idx, task.elements);
				for (uint32 i = range.first; i < range.second; i++)
					function(data[i]);
			};
			tsk.elements = numeric_cast<uint32>(data->size());
			tsk.invocations = aggInvocations<Aggregation>(tsk.elements);
			tsk.data = std::move(data).template cast<void>();
			return tasksRunImpl<Async>(std::move(tsk));
		}

		template<class T, uint32 Aggregation, bool Async>
		auto tasksRunImpl(StringPointer name, Holder<PointerRange<T>> data)
		{
			TaskCreateConfig tsk;
			tsk.name = name;
			tsk.runner = +[](const TaskCreateConfig &task, uint32 idx)
			{
				const PointerRange<T> &data = *static_cast<PointerRange<T> *>(+task.data);
				const auto range = aggRange<Aggregation>(idx, task.elements);
				for (uint32 i = range.first; i < range.second; i++)
					data[i]();
			};
			tsk.elements = numeric_cast<uint32>(data->size());
			tsk.invocations = aggInvocations<Aggregation>(tsk.elements);
			tsk.data = std::move(data).template cast<void>();
			return tasksRunImpl<Async>(std::move(tsk));
		}

		template<class T, bool Async>
		auto tasksRunImpl(StringPointer name, Delegate<void(T &, uint32)> function, Holder<T> data, uint32 invocations)
		{
			static_assert(sizeof(privat::TaskCreateConfig::function) == sizeof(function));
			TaskCreateConfig tsk;
			tsk.name = name;
			tsk.function = reinterpret_cast<Delegate<void()> &>(function);
			tsk.runner = +[](const TaskCreateConfig &task, uint32 idx)
			{
				const Delegate<void(T &, uint32)> function = reinterpret_cast<const Delegate<void(T &, uint32)> &>(task.function);
				T *data = static_cast<T *>(+task.data);
				function(*data, idx);
			};
			tsk.invocations = invocations;
			tsk.data = std::move(data).template cast<void>();
			return tasksRunImpl<Async>(std::move(tsk));
		}

		template<class T, bool Async>
		auto tasksRunImpl(StringPointer name, Holder<T> data, uint32 invocations)
		{
			TaskCreateConfig tsk;
			tsk.name = name;
			tsk.runner = +[](const TaskCreateConfig &task, uint32 idx)
			{
				T *data = static_cast<T *>(+task.data);
				(*data)(idx);
			};
			tsk.invocations = invocations;
			tsk.data = std::move(data).template cast<void>();
			return tasksRunImpl<Async>(std::move(tsk));
		}
	}

	// invoke the function once for each element of the range
	template<class T, uint32 Aggregation = 1>
	CAGE_FORCE_INLINE void tasksRunBlocking(StringPointer name, Delegate<void(T &)> function, PointerRange<T> data)
	{
		return privat::tasksRunImpl<T, Aggregation, false>(name, function, Holder<PointerRange<T>>(&data, nullptr));
	}
	template<class T, uint32 Aggregation = 1>
	[[nodiscard]] CAGE_FORCE_INLINE Holder<AsyncTask> tasksRunAsync(StringPointer name, Delegate<void(T &)> function, Holder<PointerRange<T>> data)
	{
		return privat::tasksRunImpl<T, Aggregation, true>(name, function, std::move(data));
	}

	// invoke operator()() once for each element of the range
	template<class T, uint32 Aggregation = 1>
	CAGE_FORCE_INLINE void tasksRunBlocking(StringPointer name, PointerRange<T> data)
	{
		return privat::tasksRunImpl<T, Aggregation, false>(name, Holder<PointerRange<T>>(&data, nullptr));
	}
	template<class T, uint32 Aggregation = 1>
	[[nodiscard]] CAGE_FORCE_INLINE Holder<AsyncTask> tasksRunAsync(StringPointer name, Holder<PointerRange<T>> data)
	{
		return privat::tasksRunImpl<T, Aggregation, true>(name, std::move(data));
	}

	// invoke the function invocations time, each time with the same data
	template<class T>
	CAGE_FORCE_INLINE void tasksRunBlocking(StringPointer name, Delegate<void(T &, uint32)> function, T &data, uint32 invocations)
	{
		return privat::tasksRunImpl<T, false>(name, function, Holder<T>(&data, nullptr), invocations);
	}
	template<class T>
	[[nodiscard]] CAGE_FORCE_INLINE Holder<AsyncTask> tasksRunAsync(StringPointer name, Delegate<void(T &, uint32)> function, Holder<T> data, uint32 invocations = 1)
	{
		return privat::tasksRunImpl<T, true>(name, function, std::move(data), invocations);
	}

	// invoke operator()(uint32) invocations time, each time with the same data
	template<class T>
	CAGE_FORCE_INLINE void tasksRunBlocking(StringPointer name, T &data, uint32 invocations)
	{
		return privat::tasksRunImpl<T, false>(name, Holder<T>(&data, nullptr), invocations);
	}
	template<class T>
	[[nodiscard]] CAGE_FORCE_INLINE Holder<AsyncTask> tasksRunAsync(StringPointer name, Holder<T> data, uint32 invocations = 1)
	{
		return privat::tasksRunImpl<T, true>(name, std::move(data), invocations);
	}

	// invoke the function invocations time
	CAGE_CORE_API void tasksRunBlocking(StringPointer name, Delegate<void(uint32)> function, uint32 invocations);
	[[nodiscard]] CAGE_CORE_API Holder<AsyncTask> tasksRunAsync(StringPointer name, Delegate<void(uint32)> function, uint32 invocations = 1);

	// divide tasks into groups - find begin/end indices for a particular group
	[[nodiscard]] CAGE_CORE_API std::pair<uint32, uint32> tasksSplit(uint32 groupIndex, uint32 groupsCount, uint32 tasksCount);
}

#endif // guard_tasks_h_vbnmf15dvt89zh
