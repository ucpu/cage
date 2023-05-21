#ifndef guard_events_h_h544fdjh44jjh9s
#define guard_events_h_h544fdjh44jjh9s

#include "core.h"

namespace cage
{
	template<class> struct EventListener;
	template<class> struct EventDispatcher;

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
	struct EventListener<bool(Ts...)> : private privat::EventLinker, private Delegate<bool(Ts...)>
	{
		CAGE_FORCE_INLINE explicit EventListener(const std::source_location &location = std::source_location::current()) : privat::EventLinker(location)
		{}

		CAGE_FORCE_INLINE void attach(EventDispatcher<bool(Ts...)> &dispatcher, sint32 order = 0)
		{
			privat::EventLinker::attach(&dispatcher, order);
		}

		using privat::EventLinker::detach;
		using Delegate<bool(Ts...)>::bind;
		using Delegate<bool(Ts...)>::clear;

	private:
		CAGE_FORCE_INLINE bool invoke(Ts... vs) const
		{
			auto &d = (Delegate<bool(Ts...)>&)*this;
			if (d)
				return d(std::forward<Ts>(vs)...);
			return false;
		}

		using privat::EventLinker::logAllNames;
		friend struct EventDispatcher<bool(Ts...)>;
	};

	template<class... Ts>
	struct EventListener<void(Ts...)> : private EventListener<bool(Ts...)>, private Delegate<void(Ts...)>
	{
		CAGE_FORCE_INLINE explicit EventListener(const std::source_location &location = std::source_location::current()) : EventListener<bool(Ts...)>(location)
		{
			EventListener<bool(Ts...)>::template bind<EventListener, &EventListener::invoke>(this);
		}

		using EventListener<bool(Ts...)>::attach;
		using EventListener<bool(Ts...)>::detach;
		using Delegate<void(Ts...)>::bind;
		using Delegate<void(Ts...)>::clear;

	private:
		CAGE_FORCE_INLINE bool invoke(Ts... vs) const
		{
			auto &d = (Delegate<void(Ts...)>&)*this;
			if (d)
				d(std::forward<Ts>(vs)...);
			return false;
		}
	};

	template<class... Ts>
	struct EventDispatcher<bool(Ts...)> : private EventListener<bool(Ts...)>
	{
		CAGE_FORCE_INLINE explicit EventDispatcher(const std::source_location &location = std::source_location::current()) : EventListener<bool(Ts...)>(location)
		{}

		bool dispatch(Ts... vs) const
		{
			CAGE_ASSERT(!this->p);
			const privat::EventLinker *l = this->n;
			while (l)
			{
				if (static_cast<const EventListener<bool(Ts...)>*>(l)->invoke(vs...))
					return true;
				l = l->n;
			}
			return false;
		}

		template<class Callable> requires(std::is_invocable_r_v<bool, Callable, Ts...> || std::is_invocable_r_v<void, Callable, Ts...>)
		[[nodiscard]] CAGE_FORCE_INLINE auto listen(Callable &&callable, sint32 order = 0)
		{
			struct Listener : private Immovable
			{
				EventListener<bool(Ts...)> el;
				Callable cl;

				CAGE_FORCE_INLINE explicit Listener(Callable &&callable, EventDispatcher<bool(Ts...)> &disp, sint32 order) : cl(std::move(callable))
				{
					el.attach(disp, order);
					el.template bind<Listener, &Listener::call>(this);
				}

				CAGE_FORCE_INLINE bool call(Ts... vs) const
				{
					if constexpr (std::is_same_v<decltype(cl(std::forward<Ts>(vs)...)), void>)
					{
						cl(std::forward<Ts>(vs)...);
						return false;
					}
					else
						return cl(std::forward<Ts>(vs)...);
				}
			};

			return Listener(std::move(callable), *this, order);
		}

		using EventListener<bool(Ts...)>::detach;
		using EventListener<bool(Ts...)>::logAllNames;

	private:
		friend struct EventListener<bool(Ts...)>;
	};
}

#endif // guard_events_h_h544fdjh44jjh9s
