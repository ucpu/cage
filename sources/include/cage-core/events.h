#ifndef guard_events_h_h544fdjh44jjh9s
#define guard_events_h_h544fdjh44jjh9s

#include <cage-core/core.h>

namespace cage
{
	template<class>
	struct EventListener;
	template<class>
	struct EventDispatcher;

	namespace privat
	{
		struct CAGE_CORE_API EventLinker : private Immovable
		{
			explicit EventLinker(const std::source_location &location);
			~EventLinker();

			void attach(EventLinker *d, sint32 order);
			void detach();
			void logAllNames();

			EventLinker *p = nullptr, *n = nullptr;

		private:
			void unlink();
			bool valid() const;

			const std::source_location location;
			sint32 order = 0;
		};
	}

	template<class... Ts>
	struct EventListener<bool(Ts...)> : private privat::EventLinker
	{
		CAGE_FORCE_INLINE explicit EventListener(const std::source_location &location = std::source_location::current()) : privat::EventLinker(location) {}

		template<class Callable>
		requires(std::is_invocable_r_v<bool, Callable, Ts...> || std::is_invocable_r_v<void, Callable, Ts...>)
		CAGE_FORCE_INLINE explicit EventListener(Callable &&callable, EventDispatcher<bool(Ts...)> &dispatcher, sint32 order = 0, const std::source_location &location = std::source_location::current()) : privat::EventLinker(location)
		{
			bind(std::move(callable));
			attach(dispatcher, order);
		}

		template<class Callable>
		requires(std::is_invocable_r_v<bool, Callable, Ts...> || std::is_invocable_r_v<void, Callable, Ts...>)
		CAGE_FORCE_INLINE void bind(Callable &&callable)
		{
			if constexpr (std::is_same_v<std::invoke_result_t<Callable, Ts...>, bool>)
			{
				del.b = std::move(callable);
				vd = false;
			}
			else
			{
				del.v = std::move(callable);
				vd = true;
			}
		}

		CAGE_FORCE_INLINE void clear() { del.b.clear(); }

		CAGE_FORCE_INLINE void attach(EventDispatcher<bool(Ts...)> &dispatcher, sint32 order = 0) { privat::EventLinker::attach(&dispatcher, order); }

		using privat::EventLinker::detach;

	private:
		CAGE_FORCE_INLINE bool invoke(Ts... vs) const
		{
			if (!del.b)
				return false;
			if (vd)
			{
				(del.v)(std::forward<Ts>(vs)...);
				return false;
			}
			else
				return (del.b)(std::forward<Ts>(vs)...);
		}

		union Del
		{
			Delegate<bool(Ts...)> b;
			Delegate<void(Ts...)> v;
			CAGE_FORCE_INLINE Del() : b(){};
		} del;
		bool vd = false;

		friend struct EventDispatcher<bool(Ts...)>;
	};

	template<class... Ts>
	struct EventDispatcher<bool(Ts...)> : private privat::EventLinker
	{
		CAGE_FORCE_INLINE explicit EventDispatcher(const std::source_location &location = std::source_location::current()) : privat::EventLinker(location) {}

		bool dispatch(Ts... vs) const
		{
			CAGE_ASSERT(!this->p);
			const privat::EventLinker *l = this->n;
			while (l)
			{
				if (static_cast<const EventListener<bool(Ts...)> *>(l)->invoke(vs...))
					return true;
				l = l->n;
			}
			return false;
		}

		template<class Callable>
		requires(std::is_invocable_r_v<bool, Callable, Ts...> || std::is_invocable_r_v<void, Callable, Ts...>)
		[[nodiscard]] CAGE_FORCE_INLINE auto listen(Callable &&callable, sint32 order = 0, const std::source_location &location = std::source_location::current())
		{
			return EventListener<bool(Ts...)>(std::move(callable), *this, order, location);
		}

		using privat::EventLinker::detach;
		using privat::EventLinker::logAllNames;

	private:
		friend struct EventListener<bool(Ts...)>;
	};
}

#endif // guard_events_h_h544fdjh44jjh9s
