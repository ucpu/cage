#ifndef guard_events_h_h544fdjh44jjh9s
#define guard_events_h_h544fdjh44jjh9s

#include "core.h"

#include <source_location>

namespace cage
{
	namespace privat
	{
		struct CAGE_CORE_API EventLinker : private Immovable
		{
			explicit EventLinker(const std::source_location location);
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
		explicit EventListener(const std::source_location location = std::source_location::current()) : privat::EventLinker(location)
		{}

		void attach(EventDispatcher<bool(Ts...)> &dispatcher, sint32 order = 0)
		{
			privat::EventLinker::attach(&dispatcher, order);
		}

		using privat::EventLinker::detach;
		using Delegate<bool(Ts...)>::bind;
		using Delegate<bool(Ts...)>::clear;

	private:
		bool invoke(Ts... vs) const
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
		explicit EventListener(const std::source_location location = std::source_location::current()) : EventListener<bool(Ts...)>(location)
		{
			EventListener<bool(Ts...)>::template bind<EventListener, &EventListener::invoke>(this);
		}

		using EventListener<bool(Ts...)>::attach;
		using EventListener<bool(Ts...)>::detach;
		using Delegate<void(Ts...)>::bind;
		using Delegate<void(Ts...)>::clear;

	private:
		bool invoke(Ts... vs) const
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
		explicit EventDispatcher(const std::source_location location = std::source_location::current()) : EventListener<bool(Ts...)>(location)
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

		void attach(EventListener<bool(Ts...)> &listener, sint32 order = 0)
		{
			listener.attach(*this, order);
		}

		void attach(EventListener<void(Ts...)> &listener, sint32 order = 0)
		{
			listener.attach(*this, order);
		}

		using EventListener<bool(Ts...)>::detach;
		using EventListener<bool(Ts...)>::logAllNames;

	private:
		friend struct EventListener<bool(Ts...)>;
	};
}

#endif // guard_events_h_h544fdjh44jjh9s
