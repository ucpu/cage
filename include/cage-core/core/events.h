namespace cage
{
	// delegates

	template<class T>
	struct Delegate;

	template<class R, class... Ts>
	struct Delegate<R(Ts...)>
	{
	private:
		R(*fnc)(void *, Ts...) = nullptr;
		void *inst = nullptr;

	public:
		template<R(*F)(Ts...)>
		Delegate &bind() noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				(void)inst;
				return F(templates::forward<Ts>(vs)...);
			};
			return *this;
		}

		template<class D, R(*F)(D, Ts...)>
		Delegate &bind(D d) noexcept
		{
			static_assert(sizeof(d) <= sizeof(inst), "invalid size of data stored in delegate");
			union U
			{
				void *p;
				D d;
			};
			fnc = +[](void *inst, Ts... vs) {
				U u;
				u.p = inst;
				return F(u.d , templates::forward<Ts>(vs)...);
			};
			U u;
			u.d = d;
			inst = u.p;
			return *this;
		}

		template<class C, R(C::*F)(Ts...)>
		Delegate &bind(C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return (((C*)inst)->*F)(templates::forward<Ts>(vs)...);
			};
			inst = i;
			return *this;
		}

		template<class C, R(C::*F)(Ts...) const>
		Delegate &bind(const C *i) noexcept
		{
			fnc = +[](void *inst, Ts... vs) {
				return (((const C*)inst)->*F)(templates::forward<Ts>(vs)...);
			};
			inst = const_cast<C*>(i);
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return !!fnc;
		}

		void clear() noexcept
		{
			inst = nullptr;
			fnc = nullptr;
		}

		R operator ()(Ts... vs) const
		{
			CAGE_ASSERT(!!*this, inst, fnc);
			return fnc(inst, templates::forward<Ts>(vs)...);
		}

		bool operator == (const Delegate &other) const noexcept
		{
			return fnc == other.fnc && inst == other.inst;
		}

		bool operator != (const Delegate &other) const noexcept
		{
			return !(*this == other);
		}
	};

	// events

	template<class>
	struct EventListener;
	template<class>
	struct EventDispatcher;

	namespace privat
	{
		struct CAGE_API EventLinker : private Immovable
		{
			explicit EventLinker(const string &name);
			~EventLinker();
			void attach(EventLinker *d, sint32 order);
			void detach();
			void logAllNames();
			EventLinker *p = nullptr, *n = nullptr;
		private:
			void unlink();
			bool valid() const;
			sint32 order;
			const detail::StringBase<60> name;
		};
	}

	template<class... Ts>
	struct EventListener<bool(Ts...)> : private privat::EventLinker, private Delegate<bool(Ts...)>
	{
		explicit EventListener(const string &name = "listener") : privat::EventLinker(name)
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
				return d(templates::forward<Ts>(vs)...);
			return false;
		}

		using privat::EventLinker::logAllNames;
		friend struct EventDispatcher<bool(Ts...)>;
	};

	template<class... Ts>
	struct EventListener<void(Ts...)> : private EventListener<bool(Ts...)>, private Delegate<void(Ts...)>
	{
		explicit EventListener(const string &name = "listener") : EventListener<bool(Ts...)>(name)
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
				d(templates::forward<Ts>(vs)...);
			return false;
		}
	};
	
	template<class... Ts>
	struct EventDispatcher<bool(Ts...)> : private EventListener<bool(Ts...)>
	{
		explicit EventDispatcher(const string &name = "dispatcher") : EventListener<bool(Ts...)>(name)
		{}

		bool dispatch(Ts... vs) const
		{
			CAGE_ASSERT(!this->p);
			const privat::EventLinker *l = this->n;
			while (l)
			{
				if (static_cast<const EventListener<bool(Ts...)>*>(l)->invoke(templates::forward<Ts>(vs)...))
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
