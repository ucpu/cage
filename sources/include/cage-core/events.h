#ifndef guard_events_h_h544fdjh44jjh9s
#define guard_events_h_h544fdjh44jjh9s

#include "core.h"

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
			explicit EventLinker(const String &name);
			~EventLinker();
			void attach(EventLinker *d, sint32 order);
			void detach();
			void logAllNames();
			EventLinker *p = nullptr, *n = nullptr;
		private:
			void unlink();
			bool valid() const;
			sint32 order = 0;
			const detail::StringBase<60> name;
		};
	}

	template<class... Ts>
	struct EventListener<bool(Ts...)> : private privat::EventLinker, private Delegate<bool(Ts...)>
	{
		explicit EventListener(const String &name = "listener") : privat::EventLinker(name)
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
		explicit EventListener(const String &name = "listener") : EventListener<bool(Ts...)>(name)
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
		explicit EventDispatcher(const String &name = "dispatcher") : EventListener<bool(Ts...)>(name)
		{}

		bool dispatch(Ts... vs) const
		{
			CAGE_ASSERT(!this->p);
			const privat::EventLinker *l = this->n;
			while (l)
			{
				if (static_cast<const EventListener<bool(Ts...)>*>(l)->invoke(std::forward<Ts>(vs)...))
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
